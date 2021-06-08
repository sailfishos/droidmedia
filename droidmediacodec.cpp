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

#if ANDROID_MAJOR >= 9
#include <media/stagefright/omx/1.0/Omx.h>
#elif ANDROID_MAJOR >= 8
#include <media/stagefright/omx/OMX.h>
#else
#include <media/stagefright/OMXClient.h>
#endif

// MediaCodecSource should be used instead of OMXCodec from Android 5
#if ANDROID_MAJOR >= 5
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MediaCodecSource.h>
#include <media/stagefright/MediaCodec.h>
#include <media/openmax/OMX_Video.h>
#include <media/openmax/OMX_Audio.h>
#include <media/stagefright/ACodec.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#endif

#if ANDROID_MAJOR < 7
#include <media/stagefright/OMXCodec.h>
#else
#include <media/hardware/MetadataBufferType.h>
#include "AsyncDecodingSource.h"
#endif

#if ANDROID_MAJOR >= 9
#include <media/MediaSource.h>
#else
#include <media/stagefright/MediaSource.h>
#endif
#include <media/stagefright/MetaData.h>
#include <media/stagefright/MediaDefs.h>
#include <gui/BufferQueue.h>
#include <inttypes.h>
#include "droidmediacodec.h"
#if ANDROID_MAJOR < 8
#include "allocator.h"
#endif
#include "private.h"
#include "droidmediabuffer.h"

#undef LOG_TAG
#define LOG_TAG "DroidMediaCodec"
#define DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS 5

#define SET_PARAM(k,v) \
  if (meta->v >= 0) { \
    md->setInt32(android::k, meta->v); \
  }

struct DroidMediaCodecMetaDataKey {
    const char *mime;
    int key;
    int type;
} metaDataKeys[] = {
#if ANDROID_MAJOR >= 5
    {android::MEDIA_MIMETYPE_VIDEO_HEVC, android::kKeyHVCC, android::kKeyHVCC},
#endif
    {android::MEDIA_MIMETYPE_VIDEO_MPEG4, android::kKeyESDS, android::kTypeESDS},
    {android::MEDIA_MIMETYPE_AUDIO_AAC, android::kKeyESDS, android::kTypeESDS},
    {android::MEDIA_MIMETYPE_VIDEO_AVC, android::kKeyAVCC, android::kTypeAVCC},
    {NULL, 0, 0}
};

class Buffers {
public:
    android::List<android::MediaBuffer *> buffers;
    android::Condition cond;
    android::Mutex lock;
};

class Source : public android::MediaSource {
public:
    Source(android::sp<android::MetaData>& metaData) :
        m_metaData(metaData),
        m_running(false),
        m_draining(false)
    {
    }

    void unlock() {
        m_framesReceived.lock.lock();
        m_framesReceived.cond.broadcast();
        m_framesReceived.lock.unlock();
    }

    // After this is called all new buffers will be rejected. get() will still return any already
    // queued buffers but once the queue has been emptied it will return a null buffer indicating
    // the end of the stream. There is no coming back from this.
    void drain() {
        m_framesReceived.lock.lock();
        m_draining = true;
        m_framesReceived.cond.broadcast();
        m_framesReceived.lock.unlock();
    }

    void add(android::MediaBuffer *buffer) {
        m_framesReceived.lock.lock();

        // drain buffer gets added without checking to avoid a deadlock

        while (!m_draining) {
        // TODO: I am not sure this is the right approach here.
            if (m_framesReceived.buffers.size() < DROID_MEDIA_CODEC_MAX_INPUT_BUFFERS) {
                m_framesReceived.buffers.push_back(buffer);
                m_framesReceived.cond.signal();
                m_framesReceived.lock.unlock();

                return;
            }
            // either get() will signal us or we will be signaled in case of an error
            m_framesReceived.cond.wait(m_framesReceived.lock);
        }

        m_framesReceived.lock.unlock();

        buffer->release();
    }

    android::MediaBuffer *get() {
        m_framesReceived.lock.lock();

        for (;;) {
            if (!m_running) {
                m_framesReceived.lock.unlock();
                return NULL;
            }

            if (!m_framesReceived.buffers.empty()) {
                android::List<android::MediaBuffer *>::iterator iter = m_framesReceived.buffers.begin();
                android::MediaBuffer *buffer = *iter;

                m_framesReceived.buffers.erase(iter);

                m_framesReceived.cond.signal();

                m_framesReceived.lock.unlock();

                m_framesBeingProcessed.lock.lock();
                m_framesBeingProcessed.buffers.push_back(buffer);
                m_framesBeingProcessed.lock.unlock();

                return buffer;
            }

            if (m_draining) {
                m_framesReceived.lock.unlock();

                return NULL;
            }

            m_framesReceived.cond.wait(m_framesReceived.lock);
        }

        // unreachable
        return NULL;
    }

    void removeProcessedBuffer(android::MediaBuffer *buffer) {
        m_framesBeingProcessed.lock.lock();
        for (android::List<android::MediaBuffer *>::iterator iter = m_framesBeingProcessed.buffers.begin();
             iter != m_framesBeingProcessed.buffers.end(); iter++) {
            if (*iter == buffer) {
                m_framesBeingProcessed.buffers.erase(iter);                
                m_framesBeingProcessed.cond.signal();
                m_framesBeingProcessed.lock.unlock();
                return;
            }
        }
        m_framesBeingProcessed.lock.unlock();

        ALOGW("A buffer we don't know about is being finished!");
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
    android::status_t start(android::MetaData *meta DM_UNUSED) {
        m_running = true;
        return android::OK;
    }

    android::sp<android::MetaData> getFormat() {
        // TODO: The only way to pass rotation is via a key here kKeyRotation
        return m_metaData;
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
            ALOGW("stop(): waiting for %zu frames", m_framesBeingProcessed.buffers.size());
            m_framesBeingProcessed.cond.wait(m_framesBeingProcessed.lock);
        }

        m_framesBeingProcessed.lock.unlock();

        return android::OK;
    }

#if ANDROID_MAJOR >= 9
    android::status_t read(android::MediaBufferBase **buffer,
#else
    android::status_t read(android::MediaBuffer **buffer,
#endif
                           const android::MediaSource::ReadOptions *options DM_UNUSED = NULL) {
        *buffer = get();

        if (*buffer) {
            return android::OK;
        } else {
            return android::NOT_ENOUGH_DATA;
        }
    }

    android::sp<android::MetaData> m_metaData;
    bool m_running;
    bool m_draining;
    Buffers m_framesReceived;
    Buffers m_framesBeingProcessed;
};

class InputBuffer : public android::MediaBuffer {
public:
    InputBuffer(void *data, size_t size, void *cb_data, DroidMediaCallback unref) :
        android::MediaBuffer(data, size),
        m_cb_data(cb_data),
        m_unref(unref) {
    }

    ~InputBuffer() {
    }

    void *m_cb_data;
    DroidMediaCallback m_unref;
};

struct _DroidMediaCodec : public android::MediaBufferObserver
{
    _DroidMediaCodec() :
        m_cb_data(0),
        m_data_cb_data(0) {
        memset(&m_cb, 0x0, sizeof(m_cb));
        memset(&m_data_cb, 0x0, sizeof(m_data_cb));
    }

    ~_DroidMediaCodec() {
	m_codec.clear();
	m_src.clear();
	m_queue.clear();
	m_window.clear();
	m_thread.clear();
#if ANDROID_MAJOR >= 5
	m_looper->stop();
#endif
    }

#if ANDROID_MAJOR >= 9
    void signalBufferReturned(android::MediaBufferBase *buff)
#else
    void signalBufferReturned(android::MediaBuffer *buff)
#endif
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
        int32_t width = 0, height = 0;

        if (!meta->findInt32(android::kKeyWidth, &width) ||
	    !meta->findInt32(android::kKeyHeight, &height)) {
	  ALOGW("notifySizeChanged without dimensions");
	  return true;
	}

        ALOGI("notifySizeChanged: width = %d, height = %d", width, height);

        if (m_cb.size_changed) {
            return m_cb.size_changed (m_cb_data, width, height) == 0 ? true : false;
        }

        return true;
    }

    android::sp<android::MediaSource> m_codec;
    android::sp<Source> m_src;
    android::sp<DroidMediaBufferQueue> m_queue;
    android::sp<ANativeWindow> m_window;
#if ANDROID_MAJOR >= 5
    android::sp<android::ALooper> m_looper;
#endif

    android::sp<android::Thread> m_thread;

    bool m_useExternalLoop;

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
        return droid_media_codec_loop (m_codec) == DROID_MEDIA_CODEC_LOOP_OK ? true : false;
    }

private:
    DroidMediaCodec *m_codec;
};

class DroidMediaCodecBuilder {
public:
  DroidMediaCodecBuilder(DroidMediaCodecEncoderMetaData *meta) :
    m_enc(meta), m_dec(NULL) {}
  DroidMediaCodecBuilder(DroidMediaCodecDecoderMetaData *meta) :
    m_enc(NULL), m_dec(meta) {}

  android::sp<android::MediaSource> createCodec(android::sp<android::MediaSource> src,
						android::sp<ANativeWindow> window,
#if ANDROID_MAJOR >= 5
						android::sp<android::ALooper> looper = NULL,
#endif
						android::sp<android::MetaData> md = NULL
						) {

    if (md == NULL) {
      md = buildMetaData();
    }

    if (md == NULL) {
      return NULL;
    }

#if ANDROID_MAJOR >= 5
    const char *mime;
    if (!md->findCString(android::kKeyMIMEType, &mime)) {
    	ALOGE("No mime type declared for codec");
    	return NULL;
    }

    if (isEncoder()) {
      if (!strncmp("video", mime, 5)) {
      android::sp<android::AMessage> format = new android::AMessage();

      format->setString("mime", mime);
      ALOGW("Creating video encoder for %s", mime);
      int32_t width, height, stride, sliceh, colour, frames, maxinput, bitrate, iframes;

      if (md->findInt32(android::kKeyWidth, &width)) {
          format->setInt32("width", width);
      }

      if (md->findInt32(android::kKeyHeight, &height)) {
          format->setInt32("height", height);
      }

      if (md->findInt32(android::kKeyStride, &stride)) {
          format->setInt32("stride", stride);
      }

      if (md->findInt32(android::kKeySliceHeight, &sliceh)) {
          format->setInt32("slice-height", sliceh);
      }

      if (md->findInt32(android::kKeyColorFormat, &colour)) {
          format->setInt32("color-format", colour);
      }

      if (md->findInt32(android::kKeyFrameRate, &frames)) {
          format->setInt32("frame-rate", frames);
      }

      if (md->findInt32(android::kKeyMaxInputSize, &maxinput)) {
          format->setInt32("max-input-size", maxinput);
      }

      if (md->findInt32(android::kKeyBitRate, &bitrate)) {
          format->setInt32("bitrate", bitrate);
      }

      if (md->findInt32(android::kKeyIFramesInterval, &iframes)) {
          format->setInt32("i-frame-interval", iframes);
      }

#if ANDROID_MAJOR >= 7
      if (m_enc->meta_data) {
          format->setInt32("android._input-metadata-buffer-type", m_enc->meta_data);
          format->setInt32("android._store-metadata-in-buffers-output", false);
      }
      format->setInt32("android._using-recorder", 1);
#endif

      if (!strcmp (mime, android::MEDIA_MIMETYPE_VIDEO_AVC)) {
        int32_t profile = m_enc->codec_specific.h264.profile;
        int32_t level = m_enc->codec_specific.h264.level;

        if (!profile) {
          profile = OMX_VIDEO_AVCProfileBaseline;
        }

        if (!level) {
          level = android::ACodec::getAVCLevelFor(width, height, frames, bitrate,
                                                  (OMX_VIDEO_AVCPROFILEEXTTYPE)profile);
        }

        format->setInt32("profile", profile);
        format->setInt32("level", level);

        if (m_enc->codec_specific.h264.prepend_header_to_sync_frames) {
          format->setInt32("prepend-sps-pps-to-idr-frames", 1);
        }
      }
      //TODO: time-scale

#if ANDROID_MAJOR >= 6
      return android::MediaCodecSource::Create(looper, format, src, NULL, flags());
#else
      return android::MediaCodecSource::Create(looper, format, src, flags());
#endif
      }
      else {
        android::sp<android::AMessage> format = new android::AMessage();
        format->setString("mime", mime);
        ALOGW("Creating audio encoder for %s", mime);
        if (!strcmp (mime, android::MEDIA_MIMETYPE_AUDIO_AAC)) {
            format->setInt32("aac-profile", OMX_AUDIO_AACObjectLC);
        }

        int32_t maxinput, channels, samplerate, bitrate;
        if (md->findInt32(android::kKeyMaxInputSize, &maxinput)) {
            format->setInt32("max-input-size", maxinput);
        }
        if (md->findInt32(android::kKeyChannelCount, &channels)) {
            format->setInt32("channel-count", channels);
        }
        if (md->findInt32(android::kKeySampleRate, &samplerate)) {
            format->setInt32("sample-rate", samplerate);
        }
        if (md->findInt32(android::kKeyBitRate, &bitrate)) {
          format->setInt32("bitrate", bitrate);
        }

        return android::MediaCodecSource::Create(looper, format, src);
      }
    }
#endif
#if ANDROID_MAJOR < 7
  android::OMXClient omx;
  if (omx.connect() != android::OK) {
    ALOGE("Failed to connect to OMX");
    return NULL;
  }

  return android::OMXCodec::Create(omx.interface(),
       md,
       isEncoder(),
       src,
       NULL, flags(), window);
#else
    return android::AsyncDecodingSource::Create(src, flags(), window, looper);
#endif
  }

  bool isEncoder() { return m_enc != NULL; }

  DroidMediaCodecMetaData *meta() {
    if (m_enc) {
      return (DroidMediaCodecMetaData *)m_enc;
    } else if (m_dec) {
      return (DroidMediaCodecMetaData *)m_dec;
    } else {
      return NULL;
    }
  }

  android::sp<android::MetaData> buildMetaData() {
    if (isEncoder()) {
      return buildMetaData(m_enc);
    } else {
      return buildMetaData(m_dec);
    }
  }

  uint32_t flags() {
    if (isEncoder()) {
      return flags(m_enc);
    } else {
      return flags(m_dec);
    }
  }

  static uint32_t flags(DroidMediaCodecMetaData *meta, uint32_t currentFlags) {
    // We will not do any validation for the flags. Stagefright should take care of that.
    if (meta->flags & DROID_MEDIA_CODEC_SW_ONLY) {
#if ANDROID_MAJOR < 7
      currentFlags |= android::OMXCodec::kSoftwareCodecsOnly;
#else
      currentFlags |= android::MediaCodecList::kPreferSoftwareCodecs;
#endif
    }

    if (meta->flags & DROID_MEDIA_CODEC_HW_ONLY) {
#if ANDROID_MAJOR < 7
      currentFlags |= android::OMXCodec::kHardwareCodecsOnly;
#else
      currentFlags |= android::MediaCodecList::kHardwareCodecsOnly;
#endif
    }

    return currentFlags;
  }

private:
  android::sp<android::MetaData> buildMetaData(DroidMediaCodecEncoderMetaData *meta) {
    android::sp<android::MetaData> md(new android::MetaData);
    SET_PARAM(kKeyMaxInputSize, max_input_size);
    SET_PARAM(kKeyBitRate, bitrate);
    SET_PARAM(kKeyStride, stride);
    SET_PARAM(kKeySliceHeight, slice_height);
    SET_PARAM(kKeyColorFormat, color_format);

    // TODO: This is hardcoded for now. Fix it.
    md->setInt32(android::kKeyIFramesInterval, 2);

    // TODO: kKeyHFR

    return buildMetaData((DroidMediaCodecMetaData *)meta, md);
  }

  android::sp<android::MetaData> buildMetaData(DroidMediaCodecDecoderMetaData *meta) {
    android::sp<android::MetaData> md(new android::MetaData);

    if (meta->codec_data.size > 0) {
      const char *mime = ((DroidMediaCodecMetaData *)meta)->type;
      DroidMediaCodecMetaDataKey *key = metaDataKeys;

      while (key->mime) {
	if (!strcmp (key->mime, mime)) {
	  md->setData(key->key, key->type, meta->codec_data.data, meta->codec_data.size);
	  return buildMetaData((DroidMediaCodecMetaData *)meta, md);
	}
	++key;
      }

      ALOGE("No handler for codec data for %s", ((DroidMediaCodecMetaData *)meta)->type);
      return NULL;
    }

    return buildMetaData((DroidMediaCodecMetaData *)meta, md);
  }

  uint32_t flags(DroidMediaCodecEncoderMetaData *meta) {
#if ANDROID_MAJOR >= 7
    return flags((DroidMediaCodecMetaData *)meta, 0);
#else
    return flags((DroidMediaCodecMetaData *)meta, meta->meta_data ?
#if ANDROID_MAJOR >= 5
    	  android::MediaCodecSource::FLAG_USE_METADATA_INPUT
#else
		  android::OMXCodec::kStoreMetaDataInVideoBuffers
#endif
: 0);
#endif
  }

  uint32_t flags(DroidMediaCodecDecoderMetaData *meta) {
    return flags((DroidMediaCodecMetaData *)meta, 0);
  }

  android::sp<android::MetaData> buildMetaData(DroidMediaCodecMetaData *meta,
					       android::sp<android::MetaData> md) {
    md->setCString(android::kKeyMIMEType, meta->type);
    SET_PARAM(kKeyWidth, width);
    SET_PARAM(kKeyHeight, height);
    SET_PARAM(kKeyDisplayWidth, width);
    SET_PARAM(kKeyDisplayHeight, height);
    SET_PARAM(kKeyFrameRate, fps);
    SET_PARAM(kKeyChannelCount, channels);
    SET_PARAM(kKeySampleRate, sample_rate);

    return md;
  }

  DroidMediaCodecEncoderMetaData *m_enc;
  DroidMediaCodecDecoderMetaData *m_dec;
};

android::sp<android::MediaSource> droid_media_codec_create_encoder_raw(DroidMediaCodecEncoderMetaData *meta,
#if ANDROID_MAJOR >=5
							      android::sp<android::ALooper> looper,
#endif
							      android::sp<android::MediaSource> src)
{
  DroidMediaCodecBuilder builder(meta);
#if ANDROID_MAJOR >=5
  return builder.createCodec(src, NULL, looper);
#else
  return builder.createCodec(src, NULL);
#endif
}

extern "C" {

DroidMediaBufferQueue *droid_media_codec_get_buffer_queue (DroidMediaCodec *codec)
{
  return codec->m_queue.get();
}

DroidMediaCodec *droid_media_codec_create(DroidMediaCodecBuilder& builder)
{
  android::sp<android::MetaData> md(builder.buildMetaData());
  if (md == NULL) {
    return NULL;
  }

  android::sp<Source> src(new Source(md));
  android::sp<DroidMediaBufferQueue> queue;
  android::sp<ANativeWindow> window = NULL;
  DroidMediaCodecMetaData *meta = builder.meta();

  if (builder.isEncoder()
      || meta->flags & DROID_MEDIA_CODEC_SW_ONLY
      || meta->flags & DROID_MEDIA_CODEC_NO_MEDIA_BUFFER) {
    // Nothing
  } else {
    queue = new DroidMediaBufferQueue("DroidMediaCodecBufferQueue");
    window = queue->window();
#if ANDROID_MAJOR >= 7
    if (!queue->connectListener()) {
      ALOGE("Failed to connect buffer queue listener");
    return NULL;
    }
#endif
  }

#if ANDROID_MAJOR >= 5
  android::sp<android::ALooper> looper = new android::ALooper;
  looper->setName("DroidMediaCodecLoop");
  looper->start();

  android::sp<android::MediaSource> codec = builder.createCodec(src, window, looper, md);
#else
  android::sp<android::MediaSource> codec = builder.createCodec(src, window, md);
#endif

  if (codec == NULL) {
    ALOGE("Failed to create codec");
    return NULL;
  }

  DroidMediaCodec *mediaCodec = new DroidMediaCodec;
  mediaCodec->m_codec = codec;
  mediaCodec->m_src = src;
  mediaCodec->m_queue = queue;
  mediaCodec->m_window = window;
#if ANDROID_MAJOR >= 5
  mediaCodec->m_looper = looper;
#endif
  mediaCodec->m_useExternalLoop = (meta->flags & DROID_MEDIA_CODEC_USE_EXTERNAL_LOOP) ? true : false;

  return mediaCodec;
}

DroidMediaCodec *droid_media_codec_create_decoder(DroidMediaCodecDecoderMetaData *meta)
{
  DroidMediaCodecBuilder builder(meta);

  return droid_media_codec_create(builder);
}

DroidMediaCodec *droid_media_codec_create_encoder(DroidMediaCodecEncoderMetaData *meta)
{
  DroidMediaCodecBuilder builder(meta);

  return droid_media_codec_create(builder);
}

bool droid_media_codec_is_supported(DroidMediaCodecMetaData *meta, bool encoder)
{
#if ANDROID_MAJOR == 4 && ANDROID_MINOR < 2
    android::Vector<android::String8> matchingCodecs;
#elif ANDROID_MAJOR < 7
    android::Vector<android::OMXCodec::CodecNameAndQuirks> matchingCodecs;
#else
    android::Vector<android::AString> matchingCodecs;
#endif

#if ANDROID_MAJOR < 7
    android::OMXCodec::findMatchingCodecs(
            meta->type, encoder, NULL,
#else
    android::MediaCodecList::findMatchingCodecs(
            meta->type, encoder,
#endif
            DroidMediaCodecBuilder::flags(meta, 0),
            &matchingCodecs);

    return matchingCodecs.size() > 0;
}

bool droid_media_codec_start(DroidMediaCodec *codec)
{
#if ANDROID_MAJOR < 7
    if (codec->m_queue.get() != NULL) {
        if (!codec->m_queue->connectListener()) {
	    ALOGE("Failed to connect buffer queue listener");
	    return false;
	}
	android::status_t err = native_window_api_connect(codec->m_window.get(),
							  NATIVE_WINDOW_API_MEDIA);
	if (err != android::NO_ERROR) {
  	    ALOGE("Failed to connect window");
	    return false;
	}
    }
#endif

    int err = codec->m_codec->start();
    if (err != android::OK) {
        ALOGE("error 0x%x starting codec", -err);
        return false;
    }

    return true;
}

void droid_media_codec_stop(DroidMediaCodec *codec)
{
    if (codec->m_queue.get()) {
        codec->m_queue->disconnectListener();
     }

    if (codec->m_thread != NULL) {
        codec->m_thread->requestExit();
        // in case the thread is waiting in ->read()
        droid_media_codec_drain (codec);

        int err = codec->m_thread->requestExitAndWait();

        if (err != android::NO_ERROR) {
            ALOGE("Error 0x%x stopping thread", -err);
        }

        codec->m_thread.clear();
    }

    int err = codec->m_codec->stop();
    if (err != android::OK) {
        ALOGE("error 0x%x stopping codec", -err);
    }

    if (codec->m_queue.get()) {
        codec->m_queue->buffersReleased();
     }
}

void droid_media_codec_destroy(DroidMediaCodec *codec)
{
    delete codec;
}

void droid_media_codec_queue(DroidMediaCodec *codec, DroidMediaCodecData *data, DroidMediaBufferCallbacks *cb)
{
    InputBuffer *buffer = new InputBuffer(data->data.data, data->data.size, cb->data, cb->unref);
#if ANDROID_MAJOR >= 9
    buffer->meta_data().setInt32(android::kKeyIsSyncFrame, data->sync ? 1 : 0);
    buffer->meta_data().setInt64(android::kKeyTime, data->ts);
#else
    buffer->meta_data()->setInt32(android::kKeyIsSyncFrame, data->sync ? 1 : 0);
    buffer->meta_data()->setInt64(android::kKeyTime, data->ts);
#endif
    buffer->setObserver(codec);
    buffer->set_range(0, data->data.size);
    buffer->add_ref();
    codec->m_src->add(buffer);

    if (codec->m_useExternalLoop) {
        return;
    }

    // Now start our looping thread
    if (codec->m_thread == NULL) {
        codec->m_thread = new DroidMediaCodecLoop(codec);

        int err = codec->m_thread->run("DroidMediaCodecLoop");
        if (err != android::NO_ERROR) {
            ALOGE("Error 0x%x starting thread", -err);
            if (codec->m_cb.error) {
                codec->m_cb.error(codec->m_cb_data, err);
            }

            codec->m_thread.clear();
        }
    }
}

DroidMediaCodecLoopReturn droid_media_codec_loop(DroidMediaCodec *codec)
{
    int err;
    android::MediaBuffer *buffer = NULL;

#if ANDROID_MAJOR >= 9
    err = codec->m_codec->read((android::MediaBufferBase **)&buffer);
#else
    err = codec->m_codec->read(&buffer);
#endif
    if (err == android::INFO_FORMAT_CHANGED) {
        ALOGI("Format changed from codec");
	if (codec->notifySizeChanged()) {
	  return DROID_MEDIA_CODEC_LOOP_OK;
	} else {
	  return DROID_MEDIA_CODEC_LOOP_ERROR;
	}
    }
    if (err == -EWOULDBLOCK
#if ANDROID_MAJOR >= 7
	|| err == -ENODATA
#endif
	) {
      ALOGI("retry reading again. error: 0x%x", -err);
      return DROID_MEDIA_CODEC_LOOP_OK;
    }

    if (err != android::OK) {
        if (err == android::ERROR_END_OF_STREAM
#if ANDROID_MAJOR < 7
            || err == -ENODATA
#endif
            ) {
            ALOGE("Got EOS");

            if (codec->m_cb.signal_eos) {
                codec->m_cb.signal_eos (codec->m_cb_data);
            }

	    return DROID_MEDIA_CODEC_LOOP_EOS;
        } else {
            ALOGE("Error 0x%x reading from codec", -err);

            if (codec->m_cb.error) {
                codec->m_cb.error(codec->m_cb_data, err);
            }

	    codec->m_src->unlock();

	    return DROID_MEDIA_CODEC_LOOP_ERROR;
        }
    }

    if (buffer->range_length() == 0) {
        buffer->release();
        buffer = NULL;
	return DROID_MEDIA_CODEC_LOOP_OK;
    }

#if ANDROID_MAJOR >= 9
    // Obtaining graphic buffer currently disabled because of API changes
    android::sp<android::GraphicBuffer> buff = NULL;
#else
    android::sp<android::GraphicBuffer> buff = buffer->graphicBuffer();
#endif
    if (buff == NULL) {
        if (codec->m_data_cb.data_available) {
            DroidMediaCodecData data;
            data.data.data = (uint8_t *)buffer->data() + buffer->range_offset();
            data.data.size = buffer->range_length();
            data.ts = 0;
            data.decoding_ts = 0;

#if ANDROID_MAJOR >= 9
            if (!buffer->meta_data().findInt64(android::kKeyTime, &data.ts)) {
#else
            if (!buffer->meta_data()->findInt64(android::kKeyTime, &data.ts)) {
#endif
                // I really don't know what to do here and I doubt we will reach that anyway.
                ALOGE("Received a buffer without a timestamp!");
            } else {
                // Convert timestamp from useconds to nseconds
                data.ts *= 1000;
            }

#if ANDROID_MAJOR >= 9
            buffer->meta_data().findInt64(android::kKeyDecodingTime, &data.decoding_ts);
#else
            buffer->meta_data()->findInt64(android::kKeyDecodingTime, &data.decoding_ts);
#endif
            if (data.decoding_ts) {
                // Convert from usec to nsec.
                data.decoding_ts *= 1000;
            }

            int32_t sync = 0;
            data.sync = false;
#if ANDROID_MAJOR >= 9
            buffer->meta_data().findInt32(android::kKeyIsSyncFrame, &sync);
#else
            buffer->meta_data()->findInt32(android::kKeyIsSyncFrame, &sync);
#endif
            if (sync) {
                data.sync = true;
            }

            int32_t codecConfig = 0;
            data.codec_config = false;
#if ANDROID_MAJOR >= 9
            if (buffer->meta_data().findInt32(android::kKeyIsCodecConfig, &codecConfig)
#else
            if (buffer->meta_data()->findInt32(android::kKeyIsCodecConfig, &codecConfig)
#endif
                && codecConfig) {
                data.codec_config = true;
            }

            ALOGV("sync? %i, codec config? %i, ts = %" PRId64, sync, codecConfig, data.ts);

            codec->m_data_cb.data_available (codec->m_data_cb_data, &data);
        } else {
            ALOGE("non graphic buffer received. Skipping");
        }
    } else {
        int64_t timestamp = 0;
#if ANDROID_MAJOR >= 9
        if (!buffer->meta_data().findInt64(android::kKeyTime, &timestamp)) {
#else
        if (!buffer->meta_data()->findInt64(android::kKeyTime, &timestamp)) {
#endif
            // I really don't know what to do here and I doubt we will reach that anyway.
            ALOGE("Received a buffer without a timestamp!");
        } else {
            // Convert timestamp from useconds to nseconds
            native_window_set_buffers_timestamp(codec->m_window.get(), timestamp * 1000);
        }

        int err = codec->m_window->queueBuffer(codec->m_window.get(), buff.get()
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR >= 2) || ANDROID_MAJOR >= 5
				     , -1 /* TODO: Where do we get the fence from? */
#endif
);
        if (err != android::NO_ERROR) {
            ALOGE("queueBuffer failed with error 0x%d", -err);
        } else {
#if ANDROID_MAJOR >= 9
            buffer->meta_data().setInt32(android::kKeyRendered, 1);
#else
            buffer->meta_data()->setInt32(android::kKeyRendered, 1);
#endif
        }
    }

    buffer->release();
    buffer = NULL;

    return DROID_MEDIA_CODEC_LOOP_OK;
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

void droid_media_codec_get_output_info(DroidMediaCodec *codec,
				       DroidMediaCodecMetaData *info, DroidMediaRect *crop)
{
  android::sp<android::MetaData> md(codec->m_codec->getFormat());
  // TODO: is there a way to set info->flags?
  info->fps = 0; // TODO: is there a way to get that?

  md->findInt32(android::kKeyWidth, &info->width);
  md->findInt32(android::kKeyHeight, &info->height);
  md->findInt32(android::kKeyChannelCount, &info->channels);
  md->findInt32(android::kKeySampleRate, &info->sample_rate);
  md->findInt32(android::kKeyColorFormat, &info->hal_format);

  if (!md->findRect(android::kKeyCropRect, &crop->left, &crop->top, &crop->right, &crop->bottom)) {
    crop->left = crop->top = 0;
    crop->right = info->width;
    crop->bottom = info->height;
  } else {
    // No idea why but OMXCodec.cpp deducts 1 from right and bottom
    crop->right += 1;
    crop->bottom += 1;
  }
}

};
