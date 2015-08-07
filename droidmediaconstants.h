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

#ifndef DROID_MEDIA_CONSTANTS_H
#define DROID_MEDIA_CONSTANTS_H

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  int CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK;
  int CAMERA_FRAME_CALLBACK_FLAG_ONE_SHOT_MASK;
  int CAMERA_FRAME_CALLBACK_FLAG_COPY_OUT_MASK;
  int CAMERA_FRAME_CALLBACK_FLAG_NOOP;
  int CAMERA_FRAME_CALLBACK_FLAG_CAMCORDER;
  int CAMERA_FRAME_CALLBACK_FLAG_CAMERA;
  int CAMERA_FRAME_CALLBACK_FLAG_BARCODE_SCANNER;
  int CAMERA_MSG_ERROR;
  int CAMERA_MSG_SHUTTER;
  int CAMERA_MSG_FOCUS;
  int CAMERA_MSG_ZOOM;
  int CAMERA_MSG_PREVIEW_FRAME;
  int CAMERA_MSG_VIDEO_FRAME;
  int CAMERA_MSG_POSTVIEW_FRAME;
  int CAMERA_MSG_RAW_IMAGE;
  int CAMERA_MSG_COMPRESSED_IMAGE;
  int CAMERA_MSG_RAW_IMAGE_NOTIFY;
  int CAMERA_MSG_PREVIEW_METADATA;
  int CAMERA_MSG_FOCUS_MOVE;
  int CAMERA_MSG_ALL_MSGS;
  int CAMERA_CMD_START_SMOOTH_ZOOM;
  int CAMERA_CMD_STOP_SMOOTH_ZOOM;
  int CAMERA_CMD_SET_DISPLAY_ORIENTATION;
  int CAMERA_CMD_ENABLE_SHUTTER_SOUND;
  int CAMERA_CMD_PLAY_RECORDING_SOUND;
  int CAMERA_CMD_START_FACE_DETECTION;
  int CAMERA_CMD_STOP_FACE_DETECTION;
  int CAMERA_CMD_ENABLE_FOCUS_MOVE_MSG;
  int CAMERA_CMD_PING;
  int CAMERA_ERROR_UNKNOWN;
  int CAMERA_ERROR_RELEASED;
  int CAMERA_ERROR_SERVER_DIED;
  int CAMERA_FACE_DETECTION_HW;
  int CAMERA_FACE_DETECTION_SW;
} DroidMediaCameraConstants;

typedef struct {
  int HAL_PIXEL_FORMAT_YV12;
  int HAL_PIXEL_FORMAT_RAW_SENSOR;
  int HAL_PIXEL_FORMAT_YCrCb_420_SP;
} DroidMediaPixelFormatConstants;

void droid_media_camera_constants_init(DroidMediaCameraConstants *c);
void droid_media_pixel_format_constants_init(DroidMediaPixelFormatConstants *c);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_CONSTANTS_H */
