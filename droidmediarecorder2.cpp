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

#include <media/stagefright/MediaCodecConstants.h>
#include "droidmediarecorder.h"
#include "private.h"
#include "private2.h"

#undef LOG_TAG
#define LOG_TAG "DroidMediaRecorder"

struct callback_object {
    AMediaCodecBufferInfo buffer_info;
    int32_t buffer_index;

    callback_object(int32_t index, AMediaCodecBufferInfo* info)
        : buffer_info{*info}, buffer_index{index} {}

    callback_object() : buffer_index{-1} {}
};


struct _DroidMediaRecorder {
    _DroidMediaRecorder() :
        m_cb_data(0),
        m_running(false)
    {
        memset(&m_cb, 0x0, sizeof(m_cb));
    }

    callback_object get_output() {
        callback_object element;
        std::unique_lock<std::mutex> lock{m_mutex};

        while (m_running) {
            if (m_output_queue.empty()) {
                m_condition.wait(lock);
            } else {
                element = m_output_queue.front();
                m_output_queue.pop_front();
                break;
            }
        }
        return element;
    }

    media_status_t tick() {
        media_status_t status = AMEDIA_OK;
        DroidMediaCodecData data;
        size_t buffer_size;
        callback_object element;

        data.data.data = 0;
        data.data.size = 0;
        data.ts = 0;
        data.decoding_ts = 0;
        data.codec_config = false;
        data.sync = false;

        ALOGI("tick() start");
        element = get_output();

        if (element.buffer_index < 0) {
            return status;
        }

        if (element.buffer_info.flags & AMEDIACODEC_BUFFER_FLAG_END_OF_STREAM) {
            ALOGI("tick() end of stream");
            return AMEDIA_ERROR_END_OF_STREAM;
        }

        if (element.buffer_info.flags & AMEDIACODEC_BUFFER_FLAG_CODEC_CONFIG) {
            ALOGI("tick() codec config at index %i", element.buffer_index);
            data.codec_config = true;
        }

        data.ts = element.buffer_info.presentationTimeUs * 1000;
//        data.decoding_ts = element.buffer_info.presentationTimeUs * 1000;
        ALOGI("tick() get buffer at index %i ts %" PRId64, element.buffer_index, data.ts);

        if (element.buffer_info.flags & 0x1) {
            ALOGI("tick() key frame at index %i", element.buffer_index);
            data.sync = true;
        }

        uint8_t *buf = AMediaCodec_getOutputBuffer(m_codec, element.buffer_index, &buffer_size);
        ALOGI("tick() buffer offset %i size %i", element.buffer_info.offset, element.buffer_info.size);

        data.data.data = buf + element.buffer_info.offset;
        data.data.size = element.buffer_info.size;

        m_cb.data_available (m_cb_data, &data);

//        ALOGI("tick() release buffer at index %i", element.buffer_index);
        status = AMediaCodec_releaseOutputBuffer(m_codec, element.buffer_index, false);
        if (status != AMEDIA_OK) {
            ALOGI("AMediaCodec_releaseOutputBuffer failed idnex %i", element.buffer_index);
        }

        ALOGI("tick() done index %i", element.buffer_index);
        return status;
    }

    void *run() {
        media_status_t err = AMEDIA_OK;
        while (m_running && err == AMEDIA_OK) {
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

    std::mutex m_mutex;
    std::condition_variable m_condition;
    std::list<callback_object> m_output_queue;

    AMediaCodec *m_codec = NULL;
    AMediaCodecOnAsyncNotifyCallback m_codec_on_async_notify_callbacks;
    ANativeWindow *m_input_window = NULL;

    bool m_running;
    pthread_t m_thread;
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
    DroidMediaRecorder *recorder = (DroidMediaRecorder *)userdata;

    assert(index >= 0);
    ALOGI("on_async_output_available index %i flags: %i offset %i presentation time %" PRId64 " size %i",
        index, buffer_info->flags, buffer_info->offset, buffer_info->presentationTimeUs, buffer_info->size);

    std::unique_lock<std::mutex> lock{recorder->m_mutex};
    callback_object element{index, buffer_info};
    recorder->m_output_queue.push_back(element);
    recorder->m_condition.notify_all();

    ALOGI("on_async_output_available index %i done", index);
}

static void droid_media_recorder_on_async_format_changed(AMediaCodec* codec, void* userdata, AMediaFormat* format)
{
    (void)codec;
    ALOGI("on_async_format_changed (%s)", AMediaFormat_toString(format));
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
    ALOGI("recorder_create");
    media_status_t status;
    DroidMediaRecorder *recorder = new DroidMediaRecorder;
    AMediaFormat *format = NULL;

    recorder->m_cam = camera;

    recorder->m_codec = AMediaCodec_createEncoderByType(meta->parent.type);
    if (!recorder->m_codec) {
        ALOGE("Failed to create codec");
        goto fail;
    }

    ALOGI("color format %i meta->parent.type (mime) %s", meta->color_format, meta->parent.type);
    format = AMediaFormat_new();
    AMediaFormat_setString(format, AMEDIAFORMAT_KEY_MIME, meta->parent.type);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_BIT_RATE, meta->bitrate);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_WIDTH, meta->parent.width);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_HEIGHT, meta->parent.height);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_FRAME_RATE, 30 /*meta->parent.fps*/);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_SLICE_HEIGHT, meta->slice_height);
    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_STRIDE, meta->stride);

    AMediaFormat_setInt32(format, AMEDIAFORMAT_KEY_COLOR_FORMAT, COLOR_FormatSurface);

    AMediaFormat_setFloat(format, AMEDIAFORMAT_KEY_I_FRAME_INTERVAL, 1.0F);

    ALOGI("recorder_create set callback");
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

    status = AMediaCodec_configure(recorder->m_codec, format,
        recorder->m_input_window, NULL, AMEDIACODEC_CONFIGURE_FLAG_ENCODE);
    if (status != AMEDIA_OK) {
        ALOGE("Failed to configure codec");
        goto fail;
    }

    ALOGI("recorder_create create input surface");

    status = AMediaCodec_createInputSurface(recorder->m_codec, &recorder->m_input_window);
    if (status != AMEDIA_OK) {
        ALOGE("Failed to create input surface");
        goto fail;
    }

    ALOGI("recorder_create set external window %p", recorder->m_input_window);

    ANativeWindow_acquire(recorder->m_input_window);

    if (!droid_media_camera_set_external_video_window(camera, recorder->m_input_window)) {
        ALOGE("Failed to set external video window");
        goto fail;
    }

    ALOGI("recorder_create end");

    return recorder;

fail:
    ALOGE("recorder_create failed");

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
    ALOGI("recorder_destroy");

    if (recorder->m_codec) {
        AMediaCodec_delete(recorder->m_codec);
        recorder->m_codec = NULL;
    }

    delete recorder;
}

bool droid_media_recorder_start(DroidMediaRecorder *recorder)
{
    ALOGI("recorder_start");
    media_status_t status;

    recorder->m_running = true;

    status = AMediaCodec_start(recorder->m_codec);
    if (status != AMEDIA_OK) {
        return false;
    }

    droid_media_camera_start_external_recording(recorder->m_cam);

    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
    pthread_create(&recorder->m_thread, &attr, DroidMediaRecorder::ThreadWrapper, recorder);
    pthread_attr_destroy(&attr);

    ALOGI("recorder_start done");

    return true;
}

void droid_media_recorder_stop(DroidMediaRecorder *recorder)
{
    ALOGI("recorder_stop");

    recorder->m_running = false;

    void *dummy;
    pthread_join(recorder->m_thread, &dummy);

    droid_media_camera_stop_external_recording(recorder->m_cam);

    media_status_t status = AMediaCodec_stop(recorder->m_codec);
    if (status != AMEDIA_OK) {
        ALOGE("Failed to stop media codec");
    }

    ALOGI("recorder_stop done");
}

void droid_media_recorder_set_data_callbacks(DroidMediaRecorder *recorder,
					     DroidMediaCodecDataCallbacks *cb, void *data)
{
    memcpy(&recorder->m_cb, cb, sizeof(recorder->m_cb));
    recorder->m_cb_data = data;
}

};
