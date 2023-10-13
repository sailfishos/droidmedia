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

#include <utils/Condition.h>

#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraMetadataTags.h>
#include <camera/NdkCaptureRequest.h>
#include <media/hardware/MetadataBufferType.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
/*
#include <media/stagefright/CameraSource.h>
#include <media/stagefright/MetaData.h>
*/
#include "droidmediarecorder.h"
#include "private.h"
#include "private2.h"
//#include <media/stagefright/foundation/ALooper.h>

#undef LOG_TAG
#define LOG_TAG "DroidMediaRecorder"

struct _DroidMediaRecorder {
    _DroidMediaRecorder() :
        m_cb_data(0),
        m_running(false)
    {
        memset(&m_cb, 0x0, sizeof(m_cb));
    }
/*
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
*/
    DroidMediaCamera *m_cam;
    DroidMediaCodecDataCallbacks m_cb;
    void *m_cb_data;

    AImageReader *m_image_reader = NULL;
    AMediaCodec *m_codec = NULL;
    AMediaCodecOnAsyncNotifyCallback m_codec_on_async_notify_callbacks;
    ANativeWindow *m_input_window = NULL;
    bool m_running;
};

static void droid_media_recorder_on_async_input_available(AMediaCodec* codec, void* userdata, int32_t index)
{
    (void)codec;
    assert(index >= 0);
    ALOGI("on_async_input_available index %i", index);
}

static void droid_media_recorder_on_async_output_available(AMediaCodec* codec, void* userdata, int32_t index,
                                      AMediaCodecBufferInfo* buffer_info)
{
    (void)codec;
    assert(index >= 0);
    ALOGI("on_async_input_available index %i flags: %i offset %i presentation time %" PRId64 " size %i",
        index, buffer_info->flags, buffer_info->offset, buffer_info->presentationTimeUs, buffer_info->size);
}

static void droid_media_recorder_on_async_format_changed(AMediaCodec* codec, void* userdata, AMediaFormat* format)
{
    (void)codec;
    ALOGI("on_async_format_changed");
}

static void droid_media_recorder_on_async_error(AMediaCodec* codec, void* userdata, media_status_t error,
                           int32_t action_code, const char* detail)
{
    (void)codec;
    ALOGI("on_async_error error %i action_code %i detail: %s", error, action_code, detail);
}


extern "C" {
DroidMediaRecorder *droid_media_recorder_create(DroidMediaCamera *camera,
                                                DroidMediaCodecEncoderMetaData *meta)
{
    ALOGE("recorder_create");
    media_status_t status;
    DroidMediaRecorder *recorder = new DroidMediaRecorder;
    AMediaFormat *format = NULL;

    recorder->m_cam = camera;

    recorder->m_input_window = droid_media_camera_get_external_video_window(camera);
    if (!recorder->m_input_window) {
        goto fail;
    }

    recorder->m_codec = AMediaCodec_createEncoderByType(meta->parent.type);
    if (!recorder->m_codec) {
        ALOGE("Cannot create codec");
        goto fail;
    }

    format = AMediaFormat_new();
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, meta->parent.type);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, meta->bitrate);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, meta->parent.width);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, meta->parent.height);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, meta->parent.fps);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_SLICE_HEIGHT, meta->slice_height);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_STRIDE, meta->stride);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, meta->color_format);
//    AMediaFormat_setInt32(format, TBD_AMEDIACODEC_PARAMETER_KEY_MAX_B_FRAMES,
//                          mMaxBFrames);
//    AMediaFormat_setFloat(format, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 1.0F);

    status = AMediaCodec_configure(recorder->m_codec, format,
        recorder->m_input_window, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    if (status != AMEDIA_OK) {
        goto fail;
    }

    status = AMediaCodec_setInputSurface(recorder->m_codec, recorder->m_input_window);
    if (status != AMEDIA_OK) {
        goto fail;
    }

    // Set callbacks
    recorder->m_codec_on_async_notify_callbacks.onAsyncError = droid_media_recorder_on_async_error;
    recorder->m_codec_on_async_notify_callbacks.onAsyncFormatChanged = droid_media_recorder_on_async_format_changed;
    recorder->m_codec_on_async_notify_callbacks.onAsyncInputAvailable = droid_media_recorder_on_async_input_available;
    recorder->m_codec_on_async_notify_callbacks.onAsyncOutputAvailable = droid_media_recorder_on_async_output_available;

    status = AMediaCodec_setAsyncNotifyCallback(recorder->m_codec,
        recorder->m_codec_on_async_notify_callbacks, recorder);
    if (status != AMEDIA_OK) {
        goto fail;
    }

    return recorder;

fail:
    if (format) {
        AMediaFormat_delete(format);
    }

    if (recorder->m_codec) {
        AMediaCodec_delete(recorder->m_codec);
        recorder->m_codec = NULL;
    }

    delete recorder;
    return NULL;
}

void droid_media_recorder_destroy(DroidMediaRecorder *recorder)
{
    ALOGE("recorder_destroy");

    if (recorder->m_codec) {
        AMediaCodec_delete(recorder->m_codec);
        recorder->m_codec = NULL;
    }

    delete recorder;
}

bool droid_media_recorder_start(DroidMediaRecorder *recorder)
{
    ALOGE("recorder_start");
    recorder->m_running = true;
/*
    media_status_t status;

    status = AMediaCodec_start(recorder->m_codec);
    if (status != AMEDIA_OK) {
        return false;
    }

    droid_media_camera_start_external_recording(recorder->m_cam);
*/

/*
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&recorder->m_thread, &attr, DroidMediaRecorder::ThreadWrapper, recorder);
    pthread_attr_destroy(&attr);
*/
  return true;
}

void droid_media_recorder_stop(DroidMediaRecorder *recorder)
{
    ALOGE("recorder_stop");
    recorder->m_running = false;
/*
    droid_media_camera_stop_external_recording(recorder->m_cam);

    media_status_t status = AMediaCodec_stop(recorder->m_codec);
    if (status != AMEDIA_OK) {
        return false;
    }
*/

/*
    void *dummy;
    pthread_join(recorder->m_thread, &dummy);
*/
}

void droid_media_recorder_set_data_callbacks(DroidMediaRecorder *recorder,
					     DroidMediaCodecDataCallbacks *cb, void *data)
{
    memcpy(&recorder->m_cb, cb, sizeof(recorder->m_cb));
    recorder->m_cb_data = data;
}

};
