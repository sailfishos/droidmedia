/*
 * Copyright (C) 2016 Jolla Ltd.
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

#include <atomic>
#include <utils/Condition.h>
#include <media/stagefright/CameraSource.h>
#include <media/stagefright/MetaData.h>
#include "droidmediarecorder.h"
#include "private.h"
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR < 4)
#include <gui/Surface.h>
#endif
#if ANDROID_MAJOR >=5
#include <media/stagefright/foundation/ALooper.h>
#endif

#undef LOG_TAG
#define LOG_TAG "DroidMediaRecorder"

#if ANDROID_MAJOR < 12
namespace android {
#if ANDROID_MAJOR >= 9
  struct CameraSourceListener {
#else
  class CameraSourceListener {
#endif
  public:
    static int32_t getColorFormat(android::sp<android::CameraSource> src, android::CameraParameters p) {
      return src->isCameraColorFormatSupported (p) == android::NO_ERROR ? src->mColorFormat : -1;
    }
  };
};
#endif

struct _DroidMediaRecorder {
  _DroidMediaRecorder() :
    m_cb_data(0),
    m_running(false),
    m_codec_started(false),
    m_thread_started(false) {
    memset(&m_cb, 0x0, sizeof(m_cb));
  }

  android::status_t tick() {
    android::MediaBuffer *buffer;
#if ANDROID_MAJOR >= 9
    android::status_t err = m_codec->read((android::MediaBufferBase **)&buffer);
#else
    android::status_t err = m_codec->read(&buffer);
#endif

    if (err == android::OK) {
      DroidMediaCodecData data;
      data.data.data = (uint8_t *)buffer->data() + buffer->range_offset();
      data.data.size = buffer->range_length();
      data.ts = 0;
      data.decoding_ts = 0;
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

#if ANDROID_MAJOR >= 9
      if (buffer->meta_data().findInt64(android::kKeyTime, &data.ts)) {
#else
      if (buffer->meta_data()->findInt64(android::kKeyTime, &data.ts)) {
#endif
        // Convert timestamp from useconds to nseconds
        data.ts *= 1000;
      } else {
        if (!data.codec_config) ALOGE("Recorder received a buffer without a timestamp!");
      }

#if ANDROID_MAJOR >= 9
      if (buffer->meta_data().findInt64(android::kKeyDecodingTime, &data.decoding_ts)) {
#else
      if (buffer->meta_data()->findInt64(android::kKeyDecodingTime, &data.decoding_ts)) {
#endif
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

      m_cb.data_available (m_cb_data, &data);
	    
      buffer->release();
    }
    
    return err;
  }

  void *run() {
    android::status_t err = android::OK;
    while (m_running && err == android::OK) {
      err = tick();
    }
    return NULL;
  }

  static void *ThreadWrapper(void *that) {
    return static_cast<DroidMediaRecorder *>(that)->run();
  }

  DroidMediaCamera *m_cam;
  DroidMediaCodecDataCallbacks m_cb;
  void *m_cb_data;
  android::sp<android::CameraSource> m_src;
  android::sp<android::MediaSource> m_codec;
#if ANDROID_MAJOR >= 5
  android::sp<android::ALooper> m_looper = NULL;
#endif
  // Read by the recorder thread while the caller's thread clears it.
  std::atomic<bool> m_running;
  bool m_codec_started;
  bool m_thread_started;
  pthread_t m_thread;
};

extern "C" {
DroidMediaRecorder *droid_media_recorder_create(DroidMediaCamera *camera, DroidMediaCodecEncoderMetaData *meta) {

  android::Size size(meta->parent.width, meta->parent.height);
  DroidMediaRecorder *recorder = new DroidMediaRecorder;
  recorder->m_cam = camera;
  android::sp<android::Camera> cam(droid_media_camera_get_camera (camera));
#if ANDROID_MAJOR >= 5
  recorder->m_looper = new android::ALooper;
  recorder->m_looper->setName("DroidMediaRecorderLooper");
  recorder->m_looper->start();
#endif

  recorder->m_src = android::CameraSource::CreateFromCamera(cam->remote(),
							    cam->getRecordingProxy(), // proxy
							    -1, // cameraId
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR == 4) || ANDROID_MAJOR >= 5
							    android::String16("droidmedia"), // clientName
								android::Camera::USE_CALLING_UID, // clientUid
#if ANDROID_MAJOR >= 7
								android::Camera::USE_CALLING_PID, //clientPid
#endif
#endif
							    size,  // videoSize
							    meta->parent.fps, // frameRate
							    NULL // surface
#if ANDROID_MAJOR <= 11
							    , meta->meta_data // storeMetaDataInVideoBuffers
#endif
							    );

  // set metadata storage in codec according to whether the camera request was successful
#if ANDROID_MAJOR >= 7
  meta->meta_data = recorder->m_src->metaDataStoredInVideoBuffers();
#else
  meta->meta_data = recorder->m_src->isMetaDataStoredInVideoBuffers();
#endif
  // fetch the colour format
  recorder->m_src->getFormat()->findInt32(android::kKeyColorFormat, &meta->color_format);

  // Now the encoder:
#if ANDROID_MAJOR >= 5
  recorder->m_codec = droid_media_codec_create_encoder_raw(meta, recorder->m_looper, recorder->m_src);
#else
  recorder->m_codec = droid_media_codec_create_encoder_raw(meta, recorder->m_src);
#endif
  if (!recorder->m_codec.get()) {
    ALOGE("Cannot create codec");
    recorder->m_src.clear();
#if ANDROID_MAJOR >= 5
    recorder->m_looper->stop();
#endif
    delete recorder;
    return NULL;
  }

  return recorder;

}

void droid_media_recorder_destroy(DroidMediaRecorder *recorder) {
  // The recorder thread reads from the codec and hands the data to the
  // callbacks: it must be gone before any of that is freed. A caller that
  // destroys a running recorder (switching camera while recording) gets the
  // stop it skipped.
  droid_media_recorder_stop(recorder);

  recorder->m_codec.clear();
  recorder->m_src.clear();
#if ANDROID_MAJOR >= 5
  recorder->m_looper->stop();
#endif
  delete recorder;
}

bool droid_media_recorder_start(DroidMediaRecorder *recorder) {

  if (recorder->m_thread_started) {
    ALOGE("recorder already started");
    return false;
  }

  int err = recorder->m_codec->start();

  if (err != android::OK) {
    ALOGE("error 0x%x starting codec", -err);
    return false;
  }
  recorder->m_codec_started = true;
  recorder->m_running = true;

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
  err = pthread_create(&recorder->m_thread, &attr, DroidMediaRecorder::ThreadWrapper, recorder);
  pthread_attr_destroy(&attr);

  if (err != 0) {
    ALOGE("cannot create the recorder thread: %d", err);
    recorder->m_running = false;
    droid_media_recorder_stop(recorder);
    return false;
  }
  recorder->m_thread_started = true;

  return true;
}

void droid_media_recorder_stop(DroidMediaRecorder *recorder) {
  recorder->m_running = false;

  // Stop the codec before joining, as MPEG4Writer does with its sources: the
  // thread may be blocked in read() waiting for the next encoded buffer, and
  // stopping the codec is what makes that read() return.
  if (recorder->m_codec_started) {
    recorder->m_codec_started = false;
    int err = recorder->m_codec->stop();
    if (err != android::OK) {
      ALOGE("error 0x%x stopping codec", -err);
    }
  }

  if (recorder->m_thread_started) {
    recorder->m_thread_started = false;
    void *dummy;
    pthread_join(recorder->m_thread, &dummy);
  }
}

void droid_media_recorder_set_data_callbacks(DroidMediaRecorder *recorder,
					     DroidMediaCodecDataCallbacks *cb, void *data) {
  memcpy(&recorder->m_cb, cb, sizeof(recorder->m_cb));
  recorder->m_cb_data = data;
}

};
