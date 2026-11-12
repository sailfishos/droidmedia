/*
 * Copyright (C) 2026 Jolla Ltd.
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
 */

#include <atomic>
#include <inttypes.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gui/IGraphicBufferProducer.h>
#include <gui/Surface.h>
#include <media/MediaCodecBuffer.h>
#if ANDROID_MAJOR >= 11
#include <mediadrm/ICrypto.h>
#else
#include <media/ICrypto.h>
#endif
#include <media/openmax/OMX_IVCommon.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <utils/Condition.h>
#include <utils/Mutex.h>
#include <utils/Timers.h>
#include "droidmediascreencapture.h"
#include "private.h"
#include "screen_capture_service.h"

#undef LOG_TAG
#define LOG_TAG "DroidMediaScreenCapture"

#define DEFAULT_FINISH_TIMEOUT_MS 2000

// After withdrawing the Surface, keep the codec consuming a frame the compositor still queues.
#define DETACH_GRACE_MS 100

struct _DroidMediaScreenCapture {
  enum State {
    IDLE,       // created, not started
    STARTED,    // codec running, Surface published
    STOPPING,   // finish() or stop() tearing down
    STOPPED,
  };

  _DroidMediaScreenCapture(const DroidMediaCodecEncoderMetaData *meta,
                           DroidMediaScreenCaptureCallbacks *cb, void *data) :
    m_width(meta->parent.width),
    m_height(meta->parent.height),
    m_bitrate(meta->bitrate),
    m_fps(meta->parent.fps),
    m_profile(meta->codec_specific.h264.profile),
    m_level(meta->codec_specific.h264.level),
    m_target(DROID_MEDIA_SCREEN_CAPTURE_TARGET_DISPLAY),
    m_content_rect(),
    m_cb_data(data),
    m_generation(0),
    m_thread_started(false),
    m_running(false),
    m_state(IDLE),
    m_eos(false),
    m_drain_exited(false),
    m_stop_requested(false) {
    if (cb) {
      m_cb = *cb;
    } else {
      memset(&m_cb, 0x0, sizeof(m_cb));
    }
  }

  ~_DroidMediaScreenCapture() {
    m_producer.clear();
    if (m_codec != NULL) {
      m_codec->release();
    }
    if (m_looper != NULL) {
      m_looper->stop();
    }
  }

  bool configure() {
    m_looper = new android::ALooper;
    m_looper->setName("DroidMediaScreenCapture");
    m_looper->start();

    android::Vector<android::AString> components;
    android::MediaCodecList::findMatchingCodecs(android::MEDIA_MIMETYPE_VIDEO_AVC, true,
                                                android::MediaCodecList::kHardwareCodecsOnly,
                                                &components);
    for (size_t i = 0; i < components.size(); i++) {
      m_codec = android::MediaCodec::CreateByComponentName(m_looper, components[i]);
      if (m_codec != NULL) {
        ALOGI("Using %s", components[i].c_str());
        break;
      }
    }
    if (m_codec == NULL) {
      ALOGE("No hardware AVC encoder");
      return false;
    }

    android::sp<android::AMessage> format = new android::AMessage;
    format->setString("mime", android::MEDIA_MIMETYPE_VIDEO_AVC);
    format->setInt32("width", m_width);
    format->setInt32("height", m_height);
    format->setInt32("frame-rate", m_fps);
    format->setInt32("bitrate", m_bitrate);
    format->setInt32("i-frame-interval", 1);
    format->setInt32("color-format", OMX_COLOR_FormatAndroidOpaque);
    format->setInt32("prepend-sps-pps-to-idr-frames", 1);
    if (m_profile) {
      droid_media_codec_set_h264_profile_level(format.get(), m_profile, m_level,
                                               m_width, m_height, m_fps, m_bitrate);
    } else if (m_level) {
      ALOGW("Ignoring level 0x%x without a profile", m_level);
    }

    android::status_t err = m_codec->configure(format, NULL, NULL,
                                               android::MediaCodec::CONFIGURE_FLAG_ENCODE);
    if (err != android::OK) {
      ALOGE("Error 0x%x configuring %dx%d @%d fps, profile 0x%x level 0x%x", -err,
            m_width, m_height, m_fps, m_profile, m_level);
      return false;
    }

    err = m_codec->createInputSurface(&m_producer);
    if (err != android::OK) {
      ALOGE("Error 0x%x creating the input Surface", -err);
      return false;
    }

    return true;
  }

  bool start() {
    // The compositor connects to the input Surface's producer, served from this process.
    android::ProcessState::self()->startThreadPool();

    android::sp<android::IBinder> binder = android::defaultServiceManager()->checkService(
      android::String16(sailfish::ScreenCaptureService::getServiceName()));
    if (binder == NULL) {
      ALOGE("%s is not running", sailfish::ScreenCaptureService::getServiceName());
      return false;
    }
    m_service = android::interface_cast<sailfish::IScreenCaptureService>(binder);

    android::status_t err = m_codec->start();
    if (err != android::OK) {
      ALOGE("Error 0x%x starting codec", -err);
      m_service.clear();
      return false;
    }

    m_running = true;
    if (pthread_create(&m_thread, NULL, ThreadWrapper, this) != 0) {
      ALOGE("Cannot create the output thread");
      stopCodec();
      return false;
    }
    m_thread_started = true;

    // Publish the producer once the codec runs: the compositor may queue a frame at once.
    sailfish::ScreenCaptureSession session = sailfish::ScreenCaptureSession();
    session.width = m_width;
    session.height = m_height;
    session.fps = m_fps;
    session.target = m_target;
    session.content_rect = m_content_rect;
    m_generation = m_service->registerProducer(m_producer, session);
    if (m_generation == 0) {
      ALOGE("Cannot register the input Surface");
      stopCodec();
      return false;
    }

    android::Mutex::Autolock lock(m_lock);
    m_state = STARTED;
    ALOGI("Recording %dx%d @%d fps, target %d, session %" PRId64, m_width, m_height, m_fps,
          m_target, m_generation);
    return true;
  }

  // Withdraws the Surface from the compositor. Called without m_lock.
  void unregisterProducer() {
    if (m_generation == 0) {
      return;
    }

    m_service->unregisterProducer(m_generation);
    m_generation = 0;
    usleep(DETACH_GRACE_MS * 1000);
  }

  void stopCodec() {
    m_running = false;
    // Also wakes the output thread out of dequeueOutputBuffer().
    android::status_t err = m_codec->stop();
    if (err != android::OK) {
      ALOGW("Error 0x%x stopping codec", -err);
    }
    if (m_thread_started) {
      pthread_join(m_thread, NULL);
      m_thread_started = false;
    }
    m_service.clear();
  }

  // Only the thread that moved the state to STOPPING calls this. Returns whether the stream
  // ended; once it returns, stop() may return and the object be destroyed.
  bool teardown() {
    stopCodec();

    android::Mutex::Autolock lock(m_lock);
    m_state = STOPPED;
    m_cond.broadcast();
    return m_eos;
  }

  void reportError(int err) {
    if (m_cb.error) {
      m_cb.error(m_cb_data, err);
    }
  }

  void *run() {
    bool eos = false;

    while (m_running) {
      size_t index, offset, size;
      int64_t ts;
      uint32_t flags;
      android::status_t err = m_codec->dequeueOutputBuffer(&index, &offset, &size, &ts, &flags,
                                                           100000);

      if (err == android::OK) {
        android::sp<android::MediaCodecBuffer> buffer;
        err = m_codec->getOutputBuffer(index, &buffer);
        if (err != android::OK || buffer == NULL) {
          ALOGE("Error 0x%x getting output buffer %zu", -err, index);
          m_codec->releaseOutputBuffer(index);
          reportError(err != android::OK ? err : android::UNKNOWN_ERROR);
          break;
        }

        if (size > 0 && m_cb.data_available) {
          DroidMediaCodecData data;
          data.data.data = buffer->base() + offset;
          data.data.size = size;
          data.ts = ts * 1000;
          data.decoding_ts = 0;
          data.codec_config = (flags & android::MediaCodec::BUFFER_FLAG_CODECCONFIG) != 0;
          data.sync = !data.codec_config &&
            (flags & android::MediaCodec::BUFFER_FLAG_SYNCFRAME) != 0;
          m_cb.data_available(m_cb_data, &data);
        }

        m_codec->releaseOutputBuffer(index);

        if (flags & android::MediaCodec::BUFFER_FLAG_EOS) {
          eos = true;
          if (m_cb.signal_eos) {
            m_cb.signal_eos(m_cb_data);
          }
          break;
        }
      } else if (err == android::INFO_FORMAT_CHANGED) {
        android::sp<android::AMessage> format;
        if (m_codec->getOutputFormat(&format) == android::OK) {
          ALOGI("Output format %s", format->debugString().c_str());
        }
      } else if (err != -EAGAIN && err != android::INFO_OUTPUT_BUFFERS_CHANGED && m_running) {
        ALOGE("Error 0x%x dequeueing output", -err);
        reportError(err);
        break;
      }
    }

    android::Mutex::Autolock lock(m_lock);
    m_eos = eos;
    m_drain_exited = true;
    m_cond.broadcast();
    return NULL;
  }

  static void *ThreadWrapper(void *that) {
    return static_cast<DroidMediaScreenCapture *>(that)->run();
  }

  int m_width;
  int m_height;
  int m_bitrate;
  int m_fps;
  int m_profile;
  int m_level;
  int m_target;
  DroidMediaRect m_content_rect;
  DroidMediaScreenCaptureCallbacks m_cb;
  void *m_cb_data;
  android::sp<android::ALooper> m_looper;
  android::sp<android::MediaCodec> m_codec;
  android::sp<android::IGraphicBufferProducer> m_producer;
  android::sp<sailfish::IScreenCaptureService> m_service;
  int64_t m_generation;
  pthread_t m_thread;
  bool m_thread_started;
  std::atomic<bool> m_running;

  // m_lock guards the members below. Callbacks are never invoked under it.
  android::Mutex m_lock;
  android::Condition m_cond;
  State m_state;
  bool m_eos;
  bool m_drain_exited;
  bool m_stop_requested;
}; // struct _DroidMediaScreenCapture

extern "C" {

DroidMediaScreenCapture *droid_media_screen_capture_new(DroidMediaCodecEncoderMetaData *meta,
                                                        DroidMediaScreenCaptureCallbacks *cb,
                                                        void *data)
{
  if (meta->parent.width <= 0 || meta->parent.height <= 0 || meta->bitrate <= 0 ||
      meta->parent.fps <= 0) {
    ALOGE("Invalid configuration %dx%d @%d fps, %d bps", meta->parent.width,
          meta->parent.height, meta->parent.fps, meta->bitrate);
    return NULL;
  }

  DroidMediaScreenCapture *capture = new DroidMediaScreenCapture(meta, cb, data);
  if (!capture->configure()) {
    delete capture;
    return NULL;
  }

  return capture;
}

bool droid_media_screen_capture_set_target(DroidMediaScreenCapture *capture,
                                           DroidMediaScreenCaptureTarget target,
                                           DroidMediaRect content_rect)
{
  android::Mutex::Autolock lock(capture->m_lock);
  if (capture->m_state != DroidMediaScreenCapture::IDLE) {
    return false;
  }
  if (!sailfish::screenCaptureTargetValid(capture->m_width, capture->m_height, target,
                                          content_rect)) {
    ALOGE("Invalid target %d, content %d,%d-%d,%d in %dx%d", target, content_rect.left,
          content_rect.top, content_rect.right, content_rect.bottom, capture->m_width,
          capture->m_height);
    return false;
  }

  capture->m_target = target;
  capture->m_content_rect = content_rect;
  return true;
}

bool droid_media_screen_capture_start(DroidMediaScreenCapture *capture)
{
  {
    android::Mutex::Autolock lock(capture->m_lock);
    if (capture->m_state != DroidMediaScreenCapture::IDLE) {
      return false;
    }
  }

  return capture->start();
}

bool droid_media_screen_capture_finish(DroidMediaScreenCapture *capture, int timeout_ms)
{
  {
    android::Mutex::Autolock lock(capture->m_lock);
    if (capture->m_state != DroidMediaScreenCapture::STARTED) {
      return false;
    }
    capture->m_state = DroidMediaScreenCapture::STOPPING;
  }

  capture->unregisterProducer();

  // The output thread keeps running and carries the end of the stream.
  android::status_t err = capture->m_codec->signalEndOfInputStream();
  if (err != android::OK) {
    ALOGE("Error 0x%x signalling end of input", -err);
  } else {
    nsecs_t deadline = systemTime(SYSTEM_TIME_MONOTONIC) +
      ms2ns(timeout_ms > 0 ? timeout_ms : DEFAULT_FINISH_TIMEOUT_MS);
    android::Mutex::Autolock lock(capture->m_lock);
    while (!capture->m_drain_exited && !capture->m_stop_requested) {
      nsecs_t remaining = deadline - systemTime(SYSTEM_TIME_MONOTONIC);
      if (remaining <= 0) {
        break;
      }
      capture->m_cond.waitRelative(capture->m_lock, remaining);
    }
  }

  bool eos = capture->teardown();
  if (!eos) {
    ALOGW("Stopped before the end of the stream");
  }
  return eos;
}

bool droid_media_screen_capture_request_sync_frame(DroidMediaScreenCapture *capture)
{
  // m_lock keeps finish() and stop() from tearing down meanwhile.
  android::Mutex::Autolock lock(capture->m_lock);
  if (capture->m_state != DroidMediaScreenCapture::STARTED) {
    return false;
  }

  android::sp<android::AMessage> params = new android::AMessage;
  params->setInt32("request-sync", 0);
  android::status_t err = capture->m_codec->setParameters(params);
  if (err != android::OK) {
    ALOGE("Error 0x%x requesting a sync frame", -err);
    return false;
  }

  // Surface input encodes only what the renderer sends, so ask for a frame.
  capture->m_service->requestFrame(capture->m_generation);
  return true;
}

void droid_media_screen_capture_stop(DroidMediaScreenCapture *capture)
{
  {
    android::Mutex::Autolock lock(capture->m_lock);
    switch (capture->m_state) {
    case DroidMediaScreenCapture::IDLE:
    case DroidMediaScreenCapture::STOPPED:
      return;
    case DroidMediaScreenCapture::STOPPING:
      // Another thread is tearing down; cut its wait short and wait for it.
      capture->m_stop_requested = true;
      capture->m_cond.broadcast();
      while (capture->m_state != DroidMediaScreenCapture::STOPPED) {
        capture->m_cond.wait(capture->m_lock);
      }
      return;
    case DroidMediaScreenCapture::STARTED:
      capture->m_state = DroidMediaScreenCapture::STOPPING;
      break;
    }
  }

  capture->unregisterProducer();
  capture->teardown();
}

void droid_media_screen_capture_destroy(DroidMediaScreenCapture *capture)
{
  droid_media_screen_capture_stop(capture);
  delete capture;
}

}
