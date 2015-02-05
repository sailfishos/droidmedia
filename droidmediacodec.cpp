/*
 * Copyright (C) 2014 Jolla Ltd.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * Authored by: Mohammed Hassan <mohammed.hassan@jolla.com>
 */

#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/OMXClient.h>
#include <media/stagefright/OMXCodec.h>
#include <media/stagefright/MediaSource.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/MediaDefs.h>
#include <gui/BufferQueue.h>
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4
#include <gui/Surface.h>
#else
#include <gui/SurfaceTextureClient.h>
#endif
#include "droidmediacodec.h"
#include "allocator.h"
#include "private.h"
#include "mediabuffer.h"

#define DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS 5

extern "C" {
static bool droid_media_codec_read(DroidMediaCodec *codec);
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

    void unlock() {
        m_framesReceived.lock.lock();
        m_framesReceived.cond.signal();
        m_framesReceived.lock.unlock();
    }

    void add(android::MediaBuffer *buffer) {
        m_framesReceived.lock.lock();

        // TODO: I am not sure this is the right approach here.
        if (m_framesReceived.buffers.size() >= DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS) {
            // either get() will signal us or we will be signaled in case of an error
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

        if (buffer != NULL) {
            m_framesBeingProcessed.lock.lock();
            m_framesBeingProcessed.buffers.push_back(buffer);
            m_framesBeingProcessed.lock.unlock();
        }

        return buffer;
    }

    void removeProcessedBuffer(android::MediaBuffer *buffer) {
        for (android::List<android::MediaBuffer *>::iterator iter = m_framesBeingProcessed.buffers.begin();
             iter != m_framesBeingProcessed.buffers.end(); iter++) {
            if (*iter == buffer) {
                m_framesBeingProcessed.buffers.erase(iter);
                m_framesBeingProcessed.lock.lock();
                m_framesBeingProcessed.cond.signal();
                m_framesBeingProcessed.lock.unlock();
                return;
            }
        }

        ALOGW("DroidMediaCodec: A buffer we don't know about is being finished!");
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

        m_framesBeingProcessed.lock.lock();

        while (!m_framesBeingProcessed.buffers.empty()) {
            ALOGW("DroidMediaCodec::stop(): waiting for %d frames", m_framesBeingProcessed.buffers.size());
            m_framesBeingProcessed.cond.wait(m_framesBeingProcessed.lock);
        }

        m_framesBeingProcessed.lock.unlock();

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
    Buffers m_framesBeingProcessed;
};

class InputBuffer : public android::MediaBuffer {
public:
    InputBuffer(void *data, size_t size, void *cb_data, void (* unref)(void *)) :
        android::MediaBuffer(data, size),
        m_cb_data(cb_data),
        m_unref(unref) {
    }

    ~InputBuffer() {
    }

    void *m_cb_data;
    void (* m_unref)(void *);
};

class DroidMediaCodec : public android::MediaBufferObserver
{
public:
    DroidMediaCodec() :
        m_cb_data(0),
        m_data_cb_data(0) {
        memset(&m_cb, 0x0, sizeof(m_cb));
        memset(&m_data_cb, 0x0, sizeof(m_data_cb));
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

        m_src->removeProcessedBuffer(buffer);

        buffer->m_unref(buffer->m_cb_data);

        // TODO: android does that
        buff->setObserver(0);
        buff->release();
    }

    bool notifySizeChanged() {
        android::sp<android::MetaData> meta = m_codec->getFormat();
        int32_t width, height;
        int32_t left, top, right, bottom; // crop
        int32_t displayWidth = 0, displayHeight = 0, realWidth, realHeight;

        meta->findInt32(android::kKeyWidth, &width);
        meta->findInt32(android::kKeyHeight, &height);

        if (!meta->findRect(android::kKeyCropRect, &left, &top, &right, &bottom)) {
            left = top = 0;
            right = width - 1;
            bottom = height - 1;
        }

        meta->findInt32(android::kKeyDisplayWidth, &displayWidth);
        meta->findInt32(android::kKeyDisplayHeight, &displayHeight);

        if (displayWidth != 0) {
            realWidth = displayWidth;
        } else {
            realWidth = right - left + 1;
        }

        if (displayHeight != 0) {
            realHeight = displayHeight;
        } else {
            realHeight = bottom - top + 1;
        }

        ALOGI("DroidMediaCodec: notifySizeChanged: width = %d, height = %d", realWidth, realHeight);

        if (m_cb.size_changed) {
            return m_cb.size_changed (m_cb_data, realWidth, realHeight) == 0 ? true : false;
        }

        return true;
    }

    DroidMediaCodecCallbacks m_cb;
    void *m_cb_data;
    DroidMediaCodecDataCallbacks m_data_cb;
    void *m_data_cb_data;
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
    // TODO: expose this to the API
    uint32_t flags = 0;
    if (android::MediaCodecList::getInstance()->getCodecCapabilities(index, type,
                                                                     &profileLevels,
                                                                     &colorFormats
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4
								     , &flags
#endif
) != android::OK) {
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

DroidMediaCodec *droid_media_codec_create(DroidMediaCodecMetaData *meta,
                                          android::sp<android::MetaData>& md, bool is_encoder, uint32_t flags)
{
    android::OMXClient *omx = new android::OMXClient;
    if (omx->connect() != android::OK) {
        ALOGE("DroidMediaCodec: Failed to connect to OMX");
        delete omx;
        return NULL;
    }

    // We will not do any validation for the flags. Stagefright should take care of that.
    if (meta->flags & DROID_MEDIA_CODEC_SW_ONLY) {
        flags |= android::OMXCodec::kSoftwareCodecsOnly;
    }

    if (meta->flags & DROID_MEDIA_CODEC_HW_ONLY) {
        flags |= android::OMXCodec::kHardwareCodecsOnly;
    }

    android::sp<Source> src(new Source);

    md->setCString(android::kKeyMIMEType, meta->type);
    md->setInt32(android::kKeyWidth, meta->width);
    md->setInt32(android::kKeyHeight, meta->height);
    md->setInt32(android::kKeyFrameRate, meta->fps);

    android::sp<android::BufferQueue> queue;
    android::sp<BufferQueueListener> listener;
    android::sp<ANativeWindow> window;

    if (!is_encoder) {
        listener = new BufferQueueListener;
        queue = createBufferQueue("DroidMediaCodecBufferQueue", listener);

	if (!queue.get()) {
	  ALOGE("Failed to get buffer queue");
	  listener.clear();
	  delete omx;
	  return NULL;
	}

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4
	android::sp<android::IGraphicBufferProducer> texture = queue;
        window = new android::Surface(texture, true);
#else
        android::sp<android::ISurfaceTexture> texture = queue;
        window = new android::SurfaceTextureClient(texture);
#endif
    }

    android::sp<android::MediaSource> codec
        = android::OMXCodec::Create(omx->interface(),
                                    md,
                                    is_encoder,
                                    src,
                                    NULL, flags, is_encoder ? NULL : window);

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

DroidMediaCodec *droid_media_codec_create_decoder(DroidMediaCodecDecoderMetaData *meta)
{
    android::sp<android::MetaData> md(new android::MetaData);

    if (meta->codec_data_size > 0) {
      // TODO: This key has been removed from Android 4.4
      //        md->setData(android::kKeyRawCodecSpecificData, 0, meta->codec_data, meta->codec_data_size);
    }

    return droid_media_codec_create((DroidMediaCodecMetaData *)meta, md, false, 0);
}

DroidMediaCodec *droid_media_codec_create_encoder(DroidMediaCodecEncoderMetaData *meta)
{
    uint32_t flags = 0;

    android::sp<android::MetaData> md(new android::MetaData);
    md->setInt32(android::kKeyBitRate, meta->bitrate);
    md->setInt32(android::kKeyStride, meta->stride);
    md->setInt32(android::kKeySliceHeight, meta->slice_height);

    // TODO: This is hardcoded for now. Fix it.
    md->setInt32(android::kKeyIFramesInterval, 2);

    // TODO: kKeyHFR, kKeyColorFormat
    if (meta->meta_data) {
        flags |= android::OMXCodec::kStoreMetaDataInVideoBuffers;
    }

    return droid_media_codec_create((DroidMediaCodecMetaData *)meta, md, true, flags);
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
    if (codec->m_thread != NULL) {
        int err = codec->m_thread->requestExitAndWait();
        if (err != android::NO_ERROR) {
            ALOGE("DroidMediaCodec: Error 0x%x stopping thread", -err);
        }

        codec->m_thread.clear();
    }

    int err = codec->m_codec->stop();
    if (err != android::OK) {
        ALOGE("DroidMediaCodec: error 0x%x stopping codec", -err);
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
    InputBuffer *buffer = new InputBuffer(data->data.data, data->data.size, cb->data, cb->unref);
    buffer->meta_data()->setInt32(android::kKeyIsSyncFrame, data->sync ? 1 : 0);
    buffer->meta_data()->setInt64(android::kKeyTime, data->ts);
    buffer->setObserver(codec);
    buffer->set_range(0, data->data.size);
    buffer->add_ref();
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
    int err = codec->m_queue->acquireBuffer(&buffer
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4 // TODO: UGLY!
    , 0
#endif
);
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
        err = codec->m_queue->releaseBuffer(buffer.mBuf,
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4 // TODO: fix this when we do video rendering
					     buffer.mFrameNumber,
#endif
					    EGL_NO_DISPLAY, EGL_NO_SYNC_KHR
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4 // TODO: fix this when we do video rendering
					     , android::Fence::NO_FENCE
#endif
);
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

    if (err == android::INFO_FORMAT_CHANGED) {
        ALOGI("DroidMediaCodec: Format changed from codec");
        return codec->notifySizeChanged();
    }

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
                codec->m_src->unlock();
            }
        }

        return false;
    }

    if (buffer->range_length() == 0) {
        buffer->release();
        buffer = NULL;
        return true;
    }

    android::sp<android::GraphicBuffer> buff = buffer->graphicBuffer();
    if (buff == NULL) {
        if (codec->m_data_cb.data_available) {
            DroidMediaCodecData data;
            data.data.data = (uint8_t *)buffer->data() + buffer->range_offset();
            data.data.size = buffer->range_length();
            data.ts = 0;
            data.decoding_ts = 0;

            if (!buffer->meta_data()->findInt64(android::kKeyTime, &data.ts)) {
                // I really don't know what to do here and I doubt we will reach that anyway.
                ALOGE("DroidMediaCodec: Received a buffer without a timestamp!");
            } else {
                // Convert timestamp from useconds to nseconds
                data.ts *= 1000;
            }

            buffer->meta_data()->findInt64(android::kKeyDecodingTime, &data.decoding_ts);
            if (data.decoding_ts) {
                // Convert from usec to nsec.
                data.decoding_ts *= 1000;
            }

            int32_t sync = 0;
            data.sync = false;
            buffer->meta_data()->findInt32(android::kKeyIsSyncFrame, &sync);
            if (sync) {
                data.sync = true;
            }

            int32_t codecConfig = 0;
            data.codec_config = false;
            if (buffer->meta_data()->findInt32(android::kKeyIsCodecConfig, &codecConfig)
                && codecConfig) {
                data.codec_config = true;
            }

            ALOGV("DroidMediaCodec: sync? %i, codec config? %i, ts = %lli", sync, codecConfig, data.ts);

            codec->m_data_cb.data_available (codec->m_data_cb_data, &data);
        } else {
            ALOGE("DroidMediaCodec: non graphic buffer received. Skipping");
        }
    } else {
        int64_t timestamp = 0;
        if (!buffer->meta_data()->findInt64(android::kKeyTime, &timestamp)) {
            // I really don't know what to do here and I doubt we will reach that anyway.
            ALOGE("DroidMediaCodec: Received a buffer without a timestamp!");
        } else {
            // Convert timestamp from useconds to nseconds
            native_window_set_buffers_timestamp(codec->m_window.get(), timestamp * 1000);
        }

        codec->m_window->queueBuffer(codec->m_window.get(), buff.get()
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4
				     , -1 /* TODO: Where do we get the fence from? */
#endif
);
    }

    buffer->release();
    buffer = NULL;

    return true;
}

void droid_media_codec_set_callbacks(DroidMediaCodec *codec, DroidMediaCodecCallbacks *cb, void *data)
{
    memcpy(&codec->m_cb, cb, sizeof(codec->m_cb));
    codec->m_cb_data = data;
}

void droid_media_codec_set_data_callbacks(DroidMediaCodec *codec, DroidMediaCodecDataCallbacks *cb, void *data)
{
    memcpy(&codec->m_data_cb, cb, sizeof(codec->m_data_cb));
    codec->m_data_cb_data = data;
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

void droid_media_codec_drain(DroidMediaCodec *codec)
{
    // This will cause read to return error to OMX and OMX will signal EOS
    // In practice we can get any other error instead of ERROR_END_OF_STREAM
    codec->m_src->add(NULL);
}

};
