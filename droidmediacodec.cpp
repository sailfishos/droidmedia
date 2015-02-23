/*
 * Copyright (C) 2014-2015 Jolla Ltd.
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
#include "droidmediabuffer.h"

#define DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS 5

struct DroidMediaCodecMetaDataKey {
    const char *mime;
    int key;
    int type;
} metaDataKeys[] = {
    {android::MEDIA_MIMETYPE_VIDEO_MPEG4, android::kKeyESDS, android::kTypeESDS},
    {NULL, 0, 0}
};

class SourceBuffer : public DroidMediaCodecData
{
public:
    SourceBuffer() :
        m_buffer(0),
        m_queued(false)
    {
        memset(&m_cb, 0x0, sizeof (m_cb));
    }

    android::MediaBuffer *buffer() { return m_buffer; }

    void setBuffer(android::MediaBuffer *buffer, DroidMediaBufferCallbacks *cb) {
        assert(m_buffer == 0);
        m_buffer = buffer;

        if (cb) {
            memcpy(&m_cb, cb, sizeof(m_cb));
        } else {
            memset(&m_cb, 0x0, sizeof (m_cb));
            m_cb.data = NULL;
        }
    }

    bool isFree() { return m_buffer == 0; }

    void unrefData() {
        m_cb.unref(m_cb.data);
    }

    bool isQueued() { return m_queued; }
    void setQueued(bool queued) { m_queued = queued; }

private:
    android::MediaBuffer *m_buffer;

    DroidMediaBufferCallbacks m_cb;
    bool m_queued;
};

class Source : public android::MediaSource, public android::MediaBufferObserver {
public:
    Source() :
        m_running(false)
    {

    }

    DroidMediaCodecData *dequeueInputBuffer() {
        DroidMediaCodecData *data = NULL;

        m_lock.lock();

        for (int x = 0; x < DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS; x++) {
            if (m_inputBuffers[x].isFree()) {
                data = &m_inputBuffers[x];
                goto out;
            }
        }

        // No free buffers?
        m_inputBuffersCond.wait(m_lock);

        if (!m_running) {
            goto out;
        }

        // Retry again:
        for (int x = 0; x < DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS; x++) {
            if (m_inputBuffers[x].isFree()) {
                data = &m_inputBuffers[x];
                goto out;
            }
        }

    out:
        m_lock.unlock();
        return data;
    }

    void queueInputBuffer(SourceBuffer *buffer) {
        m_lock.lock();
        m_pendingBuffers.push_back(buffer->buffer());
        m_pendingBuffersCond.signal();
        m_lock.unlock();
    }

    void drain() {
        // Add a NULL buffer which will cause read() to return ERROR_END_OF_STREAM
        m_lock.lock();
        m_pendingBuffers.push_back(NULL);
        m_pendingBuffersCond.signal();
        m_lock.unlock();
    }

    void unlock() {
        m_lock.lock();
        m_inputBuffersCond.signal();
        m_lock.unlock();
    }

    void flush() {
        m_lock.lock();

        for (int x = 0; x < DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS; x++) {
            android::MediaBuffer *buff = m_inputBuffers[x].buffer();
            if (buff) {
                if (m_inputBuffers[x].isQueued()) {
                    // Buffer owned by OMX
                    continue;
                }

                m_inputBuffers[x].unrefData();

                m_inputBuffers[x].setBuffer(NULL, NULL);

                buff->setObserver(0);
                buff->release();
                buff = NULL;
            }
        }

        m_pendingBuffers.clear();

        // Signal _dequeue_input_buffer()
        m_inputBuffersCond.signal();

        m_lock.unlock();
    }

private:
    android::status_t start(android::MetaData *meta) {
        m_lock.lock();
        m_running = true;
        m_lock.unlock();

        return android::OK;
    }

    android::sp<android::MetaData> getFormat() {
        // TODO: The only way to pass rotation is via a key here kKeyRotation
        fprintf(stderr, "%s\n", __PRETTY_FUNCTION__);
        return android::sp<android::MetaData>(new android::MetaData);
    }

    android::status_t stop() {
        m_lock.lock();

        // Mark as not running
        m_running = false;

        // In case dequeueInputBuffer() is waiting
        m_pendingBuffersCond.signal();

        m_lock.unlock();

        // flush
        flush();

        m_lock.lock();
        bool queued = false;

    retry:
        for (int x = 0; x < DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS; x++) {
            if (m_inputBuffers[x].isQueued()) {
                queued = true;
                break;
            }
        }

        if (queued) {
            ALOGV("DroidMediaCodec::stop(): waiting for frames");
            m_inputBuffersCond.wait(m_lock);
            queued = false;
            goto retry;
        }

        m_lock.unlock();

        return android::OK;
    }

    SourceBuffer *findSourceBufferLocked(android::MediaBuffer *buff) {
        for (int x = 0; x < DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS; x++) {
            if (m_inputBuffers[x].buffer() == buff) {
                return &m_inputBuffers[x];
            }
        }

        return NULL;
    }

    void signalBufferReturned(android::MediaBuffer *buff)
    {
        assert(buff != NULL);

        m_lock.lock();
        SourceBuffer *src = findSourceBufferLocked(buff);
        src->unrefData();
        src->setBuffer(NULL, NULL);
        src->setQueued(false);

        m_inputBuffersCond.signal();
        buff->setObserver(0);
        buff->release();
        buff = NULL;

        m_lock.unlock();
    }

    android::status_t read(android::MediaBuffer **buffer,
                           const android::MediaSource::ReadOptions *options = NULL) {
        android::status_t ret;

        m_lock.lock();

        while (m_running && m_pendingBuffers.empty()) {
            m_pendingBuffersCond.wait(m_lock);
        }

        if (!m_running || m_pendingBuffers.empty()) {
            *buffer = NULL;
            ret = android::NOT_ENOUGH_DATA;
        } else {
            android::List<android::MediaBuffer *>::iterator iter = m_pendingBuffers.begin();
            *buffer = *iter;
            m_pendingBuffers.erase(iter);
            if (*buffer) {
                SourceBuffer *src = findSourceBufferLocked(*buffer);
                src->setQueued(true);
                ret = android::OK;
            } else {
                ret = android::ERROR_END_OF_STREAM;
            }
        }

        if (*buffer) {
            // We add the ref here because if we add it earlier and then try to delete the buffer
            // in flush() then we will happily assert
            (*buffer)->add_ref();
        }

        m_lock.unlock();

        return ret;
    }

    android::Mutex m_lock;
    android::Condition m_inputBuffersCond;
    android::Condition m_pendingBuffersCond;

    SourceBuffer m_inputBuffers[DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS];

    android::List<android::MediaBuffer *> m_pendingBuffers;

    bool m_running;
};

class DroidMediaCodec
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
    android::sp<DroidMediaBufferQueue> m_queue;
    android::sp<ANativeWindow> m_window;

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

extern "C" {

DroidMediaBufferQueue *droid_media_codec_get_buffer_queue (DroidMediaCodec *codec)
{
  return codec->m_queue.get();
}

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

    android::sp<DroidMediaBufferQueue> queue;
    android::sp<ANativeWindow> window;

    if (!is_encoder) {
        queue = new DroidMediaBufferQueue("DroidMediaCodecBufferQueue");
	if (!queue->connectListener()) {
	  ALOGE("Failed to connect buffer queue listener");
	  queue.clear();
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

	android::status_t err = native_window_api_connect(window.get(), NATIVE_WINDOW_API_MEDIA);
	if (err != android::NO_ERROR) {
	  ALOGE("DroidMediaCodec: Failed to connect window");
	  omx->disconnect();
	  delete omx;
	  return NULL;
	}
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

    return mediaCodec;
}

DroidMediaCodec *droid_media_codec_create_decoder(DroidMediaCodecDecoderMetaData *meta)
{
    android::sp<android::MetaData> md(new android::MetaData);

    if (meta->codec_data.size > 0) {
        const char *mime = ((DroidMediaCodecMetaData *)meta)->type;
        DroidMediaCodecMetaDataKey *key = metaDataKeys;
        bool found = false;

        while (key->mime) {
            if (!strcmp (key->mime, mime)) {
                md->setData(key->key, key->type, meta->codec_data.data, meta->codec_data.size);
                found = true;
                break;
            }
            ++key;
        }

        if (!found) {
            ALOGE("DroidMediaCodec: No handler for codec data for %s", ((DroidMediaCodecMetaData *)meta)->type);
            return NULL;
        }
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
    int err = codec->m_codec->stop();
    if (err != android::OK) {
        ALOGE("DroidMediaCodec: error 0x%x stopping codec", -err);
    }
}

void droid_media_codec_destroy(DroidMediaCodec *codec)
{
    codec->m_omx->disconnect();
    codec->m_codec.clear();
    codec->m_queue.clear();
    codec->m_window.clear();
    codec->m_src.clear();

    delete codec->m_omx;
    delete codec;
}

DroidMediaCodecData *droid_media_codec_dequeue_input_buffer(DroidMediaCodec *codec)
{
    return codec->m_src->dequeueInputBuffer();
}

void droid_media_codec_queue_input_buffer(DroidMediaCodec *codec, DroidMediaCodecData *data,
					  DroidMediaBufferCallbacks *cb)
{
    SourceBuffer *buffer = (SourceBuffer *)data;

    assert(buffer->buffer() == 0);

    android::MediaBuffer *mediaBuffer = new android::MediaBuffer(data->data.data, data->data.size);
    buffer->setBuffer(mediaBuffer, cb);
    mediaBuffer->meta_data()->setInt32(android::kKeyIsSyncFrame, data->sync ? 1 : 0);
    mediaBuffer->meta_data()->setInt64(android::kKeyTime, data->ts);
    mediaBuffer->setObserver(codec->m_src.get());
    mediaBuffer->set_range(0, data->data.size);

    codec->m_src->queueInputBuffer(buffer);
}

void droid_media_codec_release_input_buffer(DroidMediaCodec *codec, DroidMediaCodecData *data)
{
    // Nothing here really.
}

bool droid_media_codec_loop(DroidMediaCodec *codec)
{
    int err;
    android::MediaBuffer *buffer = NULL;
    err = codec->m_codec->read(&buffer);

    if (err == android::INFO_FORMAT_CHANGED) {
        ALOGI("DroidMediaCodec: Format changed from codec");
        return codec->notifySizeChanged();
    }

    if (err == -EWOULDBLOCK) {
      ALOGI("DroidMediaCodec: retry reading again. error: 0x%x", -err);
      return true;
    }

#if 0
    if (err == -EWOULDBLOCK || err == -ETIMEDOUT) {
      ALOGI("DroidMediaCodec: retry reading again. error: 0x%x", -err);
      return true;
    }
#endif

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

        int err = codec->m_window->queueBuffer(codec->m_window.get(), buff.get()
#if ANDROID_MAJOR == 4 && (ANDROID_MINOR == 4 || ANDROID_MINOR == 2)
				     , -1 /* TODO: Where do we get the fence from? */
#endif
);

        if (err != android::NO_ERROR) {
            ALOGE("DroidMediaCodec: queueBuffer failed with error 0x%d", -err);
        } else {
            buffer->meta_data()->setInt32(android::kKeyRendered, 1);
        }
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

void droid_media_codec_flush(DroidMediaCodec *codec)
{
    codec->m_src->flush();
}

void droid_media_codec_drain(DroidMediaCodec *codec)
{
    codec->m_src->drain();
}

};
