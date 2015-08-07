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
#endif

// From Timers.h
typedef int64_t nsecs_t; // nano-seconds
typedef struct _DroidMediaCamera DroidMediaCamera;
typedef struct _DroidMediaCameraRecordingData DroidMediaCameraRecordingData;

#define DROID_MEDIA_CAMERA_FACING_FRONT 0
#define DROID_MEDIA_CAMERA_FACING_BACK 1

typedef enum {
  DROID_MEDIA_CAMERA_FACE_DETECTION_HW = 0,
  DROID_MEDIA_CAMERA_FACE_DETECTION_SW = 1,
} DroidMediaCameraFaceDetectionType;

typedef struct {
  int facing;
  int orientation;
} DroidMediaCameraInfo;

typedef struct {
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;
  int32_t score;
  int32_t id;

  int32_t left_eye[2];
  int32_t right_eye[2];
  int32_t mouth[2];
} DroidMediaCameraFace;

typedef struct {
  void (* shutter_cb) (void *data);
  void (* focus_cb) (void *data, int arg);
  void (* focus_move_cb) (void *data, int arg);
  void (* error_cb) (void *data, int arg);
  void (* zoom_cb) (void *data, int value, int arg);

  void (* raw_image_cb) (void *data, DroidMediaData *mem);
  void (* compressed_image_cb) (void *data, DroidMediaData *mem);
  void (* postview_frame_cb) (void *data, DroidMediaData *mem);
  void (* raw_image_notify_cb) (void *data);
  void (* preview_frame_cb) (void *data, DroidMediaData *mem);

  void (* preview_metadata_cb) (void *data, const DroidMediaCameraFace *faces, size_t num_faces);
  void (* video_frame_cb) (void *data, DroidMediaCameraRecordingData *video_data);
} DroidMediaCameraCallbacks;

DroidMediaBufferQueue *droid_media_camera_get_buffer_queue (DroidMediaCamera *camera);
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
bool droid_media_camera_send_command(DroidMediaCamera *camera, int32_t cmd, int32_t arg1, int32_t arg2);
bool droid_media_camera_store_meta_data_in_buffers(DroidMediaCamera *camera, bool enabled);
void droid_media_camera_set_preview_callback_flags(DroidMediaCamera *camera, int preview_callback_flag);

bool droid_media_camera_set_parameters(DroidMediaCamera *camera, const char *params);
char *droid_media_camera_get_parameters(DroidMediaCamera *camera);

bool droid_media_camera_take_picture(DroidMediaCamera *camera, int msgType);

void droid_media_camera_release_recording_frame(DroidMediaCamera *camera, DroidMediaCameraRecordingData *data);

nsecs_t droid_media_camera_recording_frame_get_timestamp(DroidMediaCameraRecordingData *data);
size_t droid_media_camera_recording_frame_get_size(DroidMediaCameraRecordingData *data);
void *droid_media_camera_recording_frame_get_data(DroidMediaCameraRecordingData *data);

bool droid_media_camera_enable_face_detection(DroidMediaCamera *camera,
					      DroidMediaCameraFaceDetectionType type, bool enable);

int32_t droid_media_camera_get_video_color_format (DroidMediaCamera *camera);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_CAMERA_H */
