#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/MediaDefs.h>
#include <gui/BufferQueue.h>
#include <gui/SurfaceTextureClient.h>
#include "droidmediacodec.h"
#include "allocator.h"
#include "private.h"
#include "mediabuffer.h"

#define DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS 5

extern "C" {
static bool droid_media_codec_read(DroidMediaCodec *codec);
}

static void mpeg4video(android::sp<android::MetaData> md, void *codec_data, ssize_t codec_data_size) {
    // TODO:
    // This code is crap and it was ripped off android's MPEG4 writer
    uint8_t data[codec_data_size + 29];
    data[0] = data[1] = data[2] = data[3] = 0x0;
    data[4] = 0x03;
    data[5] = 23 + codec_data_size;
    data[6] = data[7] = 0x0;
    data[8] = 0x1f;
    data[9] = 0x04;
    data[10] = 15 + codec_data_size;
    data[11] = 0x20;
    data[12] = 0x11;
    data[13] = 0x01;
    data[14] = 0x77;
    data[15] = 0x00;
    data[16] = 0x00;
    data[17] = 0x03;
    data[18] = 0xe8;
    data[19] = 0x00;
    data[20] = 0x00;
    data[21] = 0x03;
    data[22] = 0xe8;
    data[23] = 0x00;
    data[24] = 0x05;
    data[25] = codec_data_size;
    memcpy(data + 26, codec_data, codec_data_size);
    data[26 + codec_data_size] = 0x06;
    data[26 + codec_data_size + 1] = 0x01;
    data[26 + codec_data_size + 2] = 0x02;
    md->setData(android::kKeyESDS, android::kTypeESDS, data + 4, codec_data_size + 29 - 4);
}

struct CodecDataConstructor {
    const char *mime;
    void (*cb)(android::sp<android::MetaData>, void *, ssize_t);
} CodecDataConstructors[] = {
    {android::MEDIA_MIMETYPE_VIDEO_MPEG4, mpeg4video},

    {NULL, NULL}
};

static void construct_codec_data(const char *mime, android::sp<android::MetaData> md,
                                 void *codec_data, ssize_t codec_data_size) {
    CodecDataConstructor *c = &CodecDataConstructors[0];
    while (c->mime != NULL) {
        if (!strcmp(c->mime, mime)) {
            c->cb(md, codec_data, codec_data_size);
            break;
        }
    }
}

class Buffers {
public:
    android::List<android::MediaBuffer *> buffers;
    android::Condition cond;
    android::Mutex lock;
};

class Source : public android::MediaSource {
public:
    Source() :
        m_running(false)
    {
    }

    void add(android::MediaBuffer *buffer) {
        m_framesReceived.lock.lock();

        // TODO: I am not sure this is the right approach here.
        if (m_framesReceived.buffers.size() >= DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS) {
            // get() will signal us.
            m_framesReceived.cond.wait(m_framesReceived.lock);
        }

        m_framesReceived.buffers.push_back(buffer);
        m_framesReceived.cond.signal();
        m_framesReceived.lock.unlock();
    }

    android::MediaBuffer *get() {
        m_framesReceived.lock.lock();

        while (m_running && m_framesReceived.buffers.empty()) {
            m_framesReceived.cond.wait(m_framesReceived.lock);
        }

        if (!m_running) {
            m_framesReceived.lock.unlock();
            return NULL;
        }

        android::List<android::MediaBuffer *>::iterator iter = m_framesReceived.buffers.begin();
        android::MediaBuffer *buffer = *iter;

        m_framesReceived.buffers.erase(iter);

        m_framesReceived.cond.signal();

        m_framesReceived.lock.unlock();

        return buffer;
    }

    void flush() {
        m_framesReceived.lock.lock();

        while (!m_framesReceived.buffers.empty()) {
            android::List<android::MediaBuffer *>::iterator iter = m_framesReceived.buffers.begin();
            android::MediaBuffer *buffer = *iter;

            m_framesReceived.buffers.erase(iter);

            if (buffer) {
                buffer->release();
            }
        }

        m_framesReceived.lock.unlock();
    }

private:
    android::status_t start(android::MetaData *meta) {
        m_running = true;
        return android::OK;
    }

    android::sp<android::MetaData> getFormat() {
        // TODO: The only way to pass rotation is via a key here kKeyRotation
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
        return android::sp<android::MetaData>(new android::MetaData);
    }

    android::status_t stop() {
        // Mark as not running
        m_framesReceived.lock.lock();
        m_running = false;

        // Just in case get() is waiting for a buffer
        m_framesReceived.cond.signal();
        m_framesReceived.lock.unlock();

        // Now clear all the buffers:
        flush();

        return android::OK;
    }

    android::status_t read(android::MediaBuffer **buffer,
                           const android::MediaSource::ReadOptions *options = NULL) {
        *buffer = get();

        if (*buffer) {
            return android::OK;
        } else {
            return android::NOT_ENOUGH_DATA;
        }
    }

    bool m_running;
    Buffers m_framesReceived;

// TODO:
//    Buffers m_framesBeingProcessed;
};

class InputBuffer : public android::MediaBuffer {
public:
    InputBuffer(void *data, size_t size, void *cb_data, void (* unref)(void *)) :
        android::MediaBuffer(data, size),
        m_cb_data(cb_data),
        m_unref(unref) {

    }

    void *m_cb_data;
    void (* m_unref)(void *);
};

class DroidMediaCodec : public android::MediaBufferObserver
{
public:
    DroidMediaCodec() :
        m_cb_data(0) {
        memset(&m_cb, 0x0, sizeof(m_cb));
    }

    android::sp<android::MediaSource> m_codec;
    android::OMXClient *m_omx;
    android::sp<Source> m_src;
    android::sp<android::BufferQueue> m_queue;
    android::sp<ANativeWindow> m_window;
    android::BufferQueue::BufferItem m_slots[android::BufferQueue::NUM_BUFFER_SLOTS];
    android::sp<BufferQueueListener> m_bufferQueueListener;
    android::sp<android::Thread> m_thread;

    void signalBufferReturned(android::MediaBuffer *buff)
    {
        InputBuffer *buffer = (InputBuffer *) buff;

        buffer->m_unref(buffer->m_cb_data);
    }

    DroidMediaCodecCallbacks m_cb;
    void *m_cb_data;
};

class DroidMediaCodecLoop : public android::Thread {
public:
    DroidMediaCodecLoop(DroidMediaCodec *codec) :
    Thread(false),
    m_codec(codec) {
    }

    bool threadLoop() {
        return droid_media_codec_read (m_codec);
    }

private:
    DroidMediaCodec *m_codec;
};

extern "C" {

ssize_t droid_media_codec_find_by_type(const char *type, bool encoder)
{
    return android::MediaCodecList::getInstance()->findCodecByType(type, encoder);
}

ssize_t droid_media_codec_find_by_name(const char *name)
{
    return android::MediaCodecList::getInstance()->findCodecByName(name);
}

size_t droid_media_codec_count()
{
    return android::MediaCodecList::getInstance()->countCodecs();
}

const char *droid_media_codec_get_name(size_t index)
{
    return android::MediaCodecList::getInstance()->getCodecName(index);
}

bool droid_media_codec_is_encoder(size_t index)
{
    return android::MediaCodecList::getInstance()->isEncoder(index);
}

bool droid_media_codec_has_quirk(size_t index, const char *quirkName)
{
    return android::MediaCodecList::getInstance()->codecHasQuirk(index, quirkName);
}

char **droid_media_codec_get_supported_types(size_t index, ssize_t *size)
{
    android::Vector<android::AString> types;
    if (android::MediaCodecList::getInstance()->getSupportedTypes(index, &types) != android::OK) {
        return NULL;
    }

    char **out = (char **)malloc(types.size() + 1);
    for (size_t x = 0; x < types.size(); x++) {
        android::AString str = types[x];
        out[x] = (char *)malloc(str.size() + 1);
        memcpy(out[x], str.c_str(), str.size());
        out[x][str.size()] = '\0';
    }

    out[types.size()] = NULL;

    *size = types.size();

    return out;
}

bool droid_media_codec_get_capabilities(size_t index, const char *type,
                                        uint32_t **profiles, uint32_t **levels, ssize_t *profiles_levels_size,
                                        uint32_t **color_formats, ssize_t *color_formats_size)
{
    android::Vector<android::MediaCodecList::ProfileLevel> profileLevels;
    android::Vector<uint32_t> colorFormats;
    if (android::MediaCodecList::getInstance()->getCodecCapabilities(index, type,
                                                                     &profileLevels,
                                                                     &colorFormats) != android::OK) {
        ALOGE("DroidMediaCodec: Failed to get codec capabilities for %s", type);
        return false;
    }

    *profiles_levels_size = profileLevels.size();
    *profiles = (uint32_t *)malloc(*profiles_levels_size * sizeof(uint32_t));
    *levels = (uint32_t *)malloc(*profiles_levels_size * sizeof(uint32_t));

    for (ssize_t x = 0; x < *profiles_levels_size; x++) {
        (*profiles)[x] = profileLevels[x].mProfile;
        (*levels)[x] = profileLevels[x].mLevel;
    }

    *color_formats_size = colorFormats.size();
    *color_formats = (uint32_t *)malloc(*color_formats_size * sizeof(uint32_t));
    for (ssize_t x = 0; x < *color_formats_size; x++) {
        (*color_formats)[x] = colorFormats[x];
    }

    return true;
}

DroidMediaCodec *droid_media_codec_create(DroidMediaCodecMetaData *meta, DroidMediaCodecFlags flags)
{
    android::OMXClient *omx = new android::OMXClient;
    if (omx->connect() != android::OK) {
        ALOGE("DroidMediaCodec: Failed to connect to OMX");
        delete omx;
        return NULL;
    }

    // We will not do any validation for the flags. Stagefright should take care of that.
    uint32_t codec_flags = 0;
    if (flags & DROID_MEDIA_CODEC_SW_ONLY) {
        codec_flags |= android::OMXCodec::kSoftwareCodecsOnly;
    }

    if (flags & DROID_MEDIA_CODEC_HW_ONLY) {
        codec_flags |= android::OMXCodec::kHardwareCodecsOnly;
    }

    if (flags & DROID_MEDIA_CODEC_STORE_META_DATA_IN_VIDEO_BUFFERS) {
        codec_flags |= android::OMXCodec::kStoreMetaDataInVideoBuffers;
    }

    android::sp<Source> src(new Source);

    android::sp<android::MetaData> md(new android::MetaData);
    md->setCString(android::kKeyMIMEType, meta->type);
    md->setInt32(android::kKeyWidth, meta->width);
    md->setInt32(android::kKeyHeight, meta->height);
    md->setInt32(android::kKeyFrameRate, meta->fps);
    if (meta->codec_data_size > 0) {
        construct_codec_data (meta->type, md, meta->codec_data, meta->codec_data_size);
    }

    android::sp<android::BufferQueue>
        queue(new android::BufferQueue(new DroidMediaAllocator, true,
                                       android::BufferQueue::MIN_UNDEQUEUED_BUFFERS));
    queue->setConsumerName(android::String8("DroidMediaCodecBufferQueue"));
    queue->setConsumerUsageBits(android::GraphicBuffer::USAGE_HW_TEXTURE);
    queue->setSynchronousMode(false);

    android::sp<BufferQueueListener> listener = new BufferQueueListener;
    if (queue->consumerConnect(listener) != android::NO_ERROR) {
        ALOGE("DroidMediaCodec: Failed to set buffer consumer");
        delete omx;
        return NULL;
    }

    android::sp<android::ISurfaceTexture> texture = queue;
    android::sp<ANativeWindow>
        window(new android::SurfaceTextureClient(texture));

    android::sp<android::MediaSource> codec
        = android::OMXCodec::Create(omx->interface(),
                                    md,
                                    flags & DROID_MEDIA_CODEC_ENCODER ? true : false,
                                    src,
                                    NULL, codec_flags, window);

    if (codec == NULL) {
        ALOGE("DroidMediaCodec: Failed to create codec");
        omx->disconnect();
        delete omx;
        return NULL;
    }

    DroidMediaCodec *mediaCodec = new DroidMediaCodec;
    mediaCodec->m_codec = codec;
    mediaCodec->m_omx = omx;
    mediaCodec->m_src = src;
    mediaCodec->m_queue = queue;
    mediaCodec->m_window = window;
    mediaCodec->m_bufferQueueListener = listener;

    return mediaCodec;
}

bool droid_media_codec_start(DroidMediaCodec *codec)
{
    int err = codec->m_codec->start();
    if (err != android::OK) {
        ALOGE("DroidMediaCodec: error 0x%x starting codec", -err);
        return false;
    }

    return true;
}

void droid_media_codec_stop(DroidMediaCodec *codec)
{
    int err = codec->m_codec->stop();
    if (err != android::OK) {
        ALOGE("DroidMediaCodec: error 0x%x stopping codec", -err);
    }

    if (codec->m_thread != NULL) {
        int err = codec->m_thread->requestExitAndWait();
        if (err != android::NO_ERROR) {
            ALOGE("DroidMediaCodec: Error 0x%x stopping thread", -err);
        }

        codec->m_thread.clear();
    }
}

void droid_media_codec_destroy(DroidMediaCodec *codec)
{
//    int err;
// TODO:
    codec->m_omx->disconnect();
/*
    err = codec->m_looper->stop();
    if (err != android::OK) {
        ALOGE("DroidMediaCodec: Error 0x%x stopping looper", -err);
    }
*/
    delete codec->m_omx;
    delete codec;
}

void droid_media_codec_write(DroidMediaCodec *codec, DroidMediaCodecData *data, DroidMediaBufferCallbacks *cb)
{
    InputBuffer *buffer = new InputBuffer(data->data, data->size, cb->data, cb->unref);
    buffer->meta_data()->setInt32(android::kKeyIsSyncFrame, data->sync ? 1 : 0);
    buffer->meta_data()->setInt64(android::kKeyTime, data->ts);
    buffer->setObserver(codec);
    buffer->set_range(0, data->size);
    buffer->add_ref();

    if (codec->m_src->m_draining) {
        ALOGE("DroidMediaCodec: received buffer while draining");
        buffer->release();
        return;
    }

    codec->m_src->add(buffer);

    // Now start our looping thread
    if (codec->m_thread == NULL) {
        codec->m_thread = new DroidMediaCodecLoop(codec);

        int err = codec->m_thread->run("DroidMediaCodecLoop");
        if (err != android::NO_ERROR) {
            ALOGE("DroidMediaCodec: Error 0x%x starting thread", -err);
            if (codec->m_cb.error) {
                codec->m_cb.error(codec->m_cb_data, err);
            }

            codec->m_thread.clear();
        }
    }
}

DroidMediaBuffer *droid_media_codec_acquire_buffer(DroidMediaCodec *codec, DroidMediaBufferCallbacks *cb)
{
    android::BufferQueue::BufferItem buffer;
    int num;
    int err = codec->m_queue->acquireBuffer(&buffer);
    if (err != android::OK) {
        ALOGE("DroidMediaCodec: Failed to acquire buffer from the queue. Error 0x%x", -err);
        return NULL;
    }

    // TODO: Here we are working around the fact that BufferQueue will send us an mGraphicBuffer
    // only when it changes. We can integrate SurfaceTexture but thart needs a lot of change in the whole stack
    num = buffer.mBuf;

    if (buffer.mGraphicBuffer != NULL) {
        codec->m_slots[num] = buffer;
    }

    if (codec->m_slots[num].mGraphicBuffer == NULL) {
        int err;
        ALOGE("DroidMediaCodec: Got a buffer without real data");
        err = codec->m_queue->releaseBuffer(buffer.mBuf, EGL_NO_DISPLAY, EGL_NO_SYNC_KHR);
        if (err != android::NO_ERROR) {
            ALOGE("DroidMediaCodec: error releasing buffer. Error 0x%x", -err);
        }

        return NULL;
    }

    codec->m_slots[num].mTransform = buffer.mTransform;
    codec->m_slots[num].mScalingMode = buffer.mScalingMode;
    codec->m_slots[num].mTimestamp = buffer.mTimestamp;
    codec->m_slots[num].mFrameNumber = buffer.mFrameNumber;
    codec->m_slots[num].mCrop = buffer.mCrop;

    return new DroidMediaBuffer(codec->m_slots[num], codec->m_queue, cb->data, cb->ref, cb->unref);
}

static bool droid_media_codec_read(DroidMediaCodec *codec)
{
    int err;
    android::MediaBuffer *buffer = NULL;
    err = codec->m_codec->read(&buffer);

    if (err != android::OK) {
        if (err == android::ERROR_END_OF_STREAM) {
            ALOGE("DroidMediaCodec: Got EOS");

            if (codec->m_cb.signal_eos) {
                codec->m_cb.signal_eos (codec->m_cb_data);
            }
        } else {
            ALOGE("DroidMediaCodec: Error 0x%x reading from codec", -err);
            if (codec->m_cb.error) {
                codec->m_cb.error(codec->m_cb_data, err);
            }
        }

        return false;
    }

    android::sp<android::GraphicBuffer> buff = buffer->graphicBuffer();
    if (buff == NULL) {
        ALOGE("DroidMediaCodec: non graphic buffer received. Skipping");
    } else {
        // TODO: timestamp
        codec->m_window->queueBuffer(codec->m_window.get(), buff.get());
    }

    buffer->release();

    return true;
}

void droid_media_codec_set_callbacks(DroidMediaCodec *codec, DroidMediaCodecCallbacks *cb, void *data)
{
    memcpy(&codec->m_cb, cb, sizeof(codec->m_cb));
    codec->m_cb_data = data;
}

void droid_media_codec_set_rendering_callbacks(DroidMediaCodec *codec,
                                               DroidMediaRenderingCallbacks *cb, void *data)
{
    codec->m_bufferQueueListener->setCallbacks(cb, data);
}

void droid_media_codec_flush(DroidMediaCodec *codec)
{
    codec->m_src->flush();
}

};
