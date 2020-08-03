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

#include "droidmedia.h"

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
  int HAL_PIXEL_FORMAT_RGBA_8888;
  int HAL_PIXEL_FORMAT_RGBX_8888;
  int HAL_PIXEL_FORMAT_RGB_888;
  int HAL_PIXEL_FORMAT_RGB_565;
  int HAL_PIXEL_FORMAT_BGRA_8888;
  int HAL_PIXEL_FORMAT_YV12;
  int HAL_PIXEL_FORMAT_RAW_SENSOR;
#if ANDROID_MAJOR < 8
  int HAL_PIXEL_FORMAT_YCrCb_420_SP;
  int HAL_PIXEL_FORMAT_YCbCr_422_SP;
  int HAL_PIXEL_FORMAT_YCbCr_422_I;
#else
  int HAL_PIXEL_FORMAT_YCRCB_420_SP;
  int HAL_PIXEL_FORMAT_YCBCR_422_SP;
  int HAL_PIXEL_FORMAT_YCBCR_422_I;
#endif
  int QOMX_COLOR_FormatYUV420PackedSemiPlanar32m;
  int QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka;
} DroidMediaPixelFormatConstants;

typedef struct {
  int QOMX_COLOR_FormatYUV420PackedSemiPlanar32m;
  int QOMX_COLOR_FormatYUV420PackedSemiPlanar64x32Tile2m8ka;
  int OMX_COLOR_FormatYUV420Planar;
  int OMX_COLOR_FormatYUV420PackedPlanar;
  int OMX_COLOR_FormatYUV420SemiPlanar;
  int OMX_COLOR_FormatYUV422SemiPlanar;
  int OMX_COLOR_FormatL8;
  int OMX_COLOR_FormatYCbYCr;
  int OMX_COLOR_FormatYCrYCb;
  int  OMX_COLOR_FormatCbYCrY;
  int OMX_COLOR_Format32bitARGB8888;
  int OMX_COLOR_Format32bitBGRA8888;
  int OMX_COLOR_Format16bitRGB565;
  int OMX_COLOR_Format16bitBGR565;
} DroidMediaColourFormatConstants;

DROID_MEDIA_EXPORT void droid_media_camera_constants_init(DroidMediaCameraConstants *c);
DROID_MEDIA_EXPORT void droid_media_pixel_format_constants_init(DroidMediaPixelFormatConstants *c);
DROID_MEDIA_EXPORT void droid_media_colour_format_constants_init(DroidMediaColourFormatConstants *c);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_CONSTANTS_H */
