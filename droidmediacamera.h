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

#ifndef DROID_MEDIA_CAMERA_H
#define DROID_MEDIA_CAMERA_H

#include <stdint.h>
#include "droidmedia.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#else
typedef void DroidMediaCamera;
typedef void DroidMediaCameraRecordingData;
#endif

// From Timers.h
typedef int64_t nsecs_t; // nano-seconds

struct DroidMediaCamera;
struct DroidMediaCameraRecordingData;

typedef struct {
  int facing;
  int orientation;
} DroidMediaCameraInfo;

typedef struct {
  void (* notify)(void *data, int32_t msgType, int32_t ext1, int32_t ext2);
  void (* post_data_timestamp)(void *data, int32_t msgType, DroidMediaCameraRecordingData *video_data);
  void (* post_data)(void *data, int32_t msgType, DroidMediaData *mem);
  // TODO: error callback
} DroidMediaCameraCallbacks;

int droid_media_camera_get_number_of_cameras();
bool droid_media_camera_get_info(DroidMediaCameraInfo *info, int camera_number);

DroidMediaCamera *droid_media_camera_connect(int camera_number);
bool droid_media_camera_reconnect(DroidMediaCamera *camera);
void droid_media_camera_disconnect(DroidMediaCamera *camera);

bool droid_media_camera_lock(DroidMediaCamera *camera);
bool droid_media_camera_unlock(DroidMediaCamera *camera);

bool droid_media_camera_start_preview(DroidMediaCamera *camera);
void droid_media_camera_stop_preview(DroidMediaCamera *camera);
bool droid_media_camera_is_preview_enabled(DroidMediaCamera *camera);

bool droid_media_camera_start_recording(DroidMediaCamera *camera);
void droid_media_camera_stop_recording(DroidMediaCamera *camera);
bool droid_media_camera_is_recording_enabled(DroidMediaCamera *camera);

bool droid_media_camera_start_auto_focus(DroidMediaCamera *camera);
bool droid_media_camera_cancel_auto_focus(DroidMediaCamera *camera);

void droid_media_camera_set_callbacks(DroidMediaCamera *camera, DroidMediaCameraCallbacks *cb, void *data);
void droid_media_camera_set_rendering_callbacks(DroidMediaCamera *camera,
						DroidMediaRenderingCallbacks *cb, void *data);
bool droid_media_camera_send_command(DroidMediaCamera *camera, int32_t cmd, int32_t arg1, int32_t arg2);
bool droid_media_camera_store_meta_data_in_buffers(DroidMediaCamera *camera, bool enabled);
void droid_media_camera_set_preview_callback_flags(DroidMediaCamera *camera, int preview_callback_flag);

bool droid_media_camera_set_parameters(DroidMediaCamera *camera, const char *params);
char *droid_media_camera_get_parameters(DroidMediaCamera *camera);

bool droid_media_camera_take_picture(DroidMediaCamera *camera, int msgType);

DroidMediaBuffer *droid_media_camera_acquire_buffer(DroidMediaCamera *camera, DroidMediaBufferCallbacks *cb);
void droid_media_camera_release_recording_frame(DroidMediaCamera *camera, DroidMediaCameraRecordingData *data);

nsecs_t droid_media_camera_recording_frame_get_timestamp(DroidMediaCameraRecordingData *data);
size_t droid_media_camera_recording_frame_get_size(DroidMediaCameraRecordingData *data);
void *droid_media_camera_recording_frame_get_data(DroidMediaCameraRecordingData *data);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_CAMERA_H */
