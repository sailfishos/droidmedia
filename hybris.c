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

#include "droidmediacamera.h"
#include "droidmediacodec.h"
#include "droidmediaconvert.h"
#include "droidmediaconstants.h"
#include <dlfcn.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef LIB_DROID_MEDIA_PATH
#define LIB_DROID_MEDIA_PATH "/usr/libexec/droid-hybris/system/lib/libdroidmedia.so"
#endif

void *android_dlopen(const char *name, int flags);
void *android_dlsym(void *handle, const char *name);

static void *__handle = NULL;

static inline void __load_library() {
  if (!__handle) {
    __handle = android_dlopen(LIB_DROID_MEDIA_PATH, RTLD_NOW);
    if (!__handle) {
      // calling abort() is bad but it does not matter anyway as we will crash.
      abort();
    }
  }
}

static inline void *__resolve_sym(const char *sym)
{
  __load_library();

  void *ptr = android_dlsym(__handle, sym);
  assert(ptr != NULL);
  if (!ptr) {
    // calling abort() is bad but it does not matter anyway as we will crash.
    abort();
  }

  return ptr;
}

#define HYBRIS_WRAPPER_1_0(ret,sym)		    \
  ret sym() {					    \
    static ret (* _sym)() = NULL;		    \
    if (!_sym)					    \
      _sym = __resolve_sym(#sym);		    \
    return _sym();				    \
  }						    \

#define HYBRIS_WRAPPER_1_1(ret,arg0,sym)     \
  ret sym(arg0 _arg0) {			     \
    static ret (* _sym)(arg0) = NULL;	     \
    if (!_sym)				     \
      _sym = __resolve_sym(#sym);	     \
    return _sym(_arg0);			     \
  }					     \

#define HYBRIS_WRAPPER_1_2(ret,arg0,arg1,sym)			     \
  ret sym(arg0 _arg0, arg1 _arg1) {				     \
    static ret (* _sym)(arg0, arg1) = NULL;			     \
    if (!_sym)							     \
      _sym = __resolve_sym(#sym);				     \
    return _sym(_arg0,_arg1);					     \
  }								     \

#define HYBRIS_WRAPPER_1_3(ret,arg0,arg1,arg2,sym)		     \
  ret sym(arg0 _arg0, arg1 _arg1, arg2 _arg2) {			     \
    static ret (* _sym)(arg0, arg1, arg2) = NULL;		     \
    if (!_sym)							     \
      _sym = __resolve_sym(#sym);				     \
    return _sym(_arg0,_arg1, _arg2);				     \
  }								     \

#define HYBRIS_WRAPPER_1_4(ret,arg0,arg1,arg2,arg3,sym)		     \
  ret sym(arg0 _arg0, arg1 _arg1, arg2 _arg2, arg3 _arg3) {	     \
    static ret (* _sym)(arg0, arg1, arg2, arg3) = NULL;		     \
    if (!_sym)							     \
      _sym = __resolve_sym(#sym);				     \
    return _sym(_arg0,_arg1, _arg2, _arg3);			     \
  }								     \

#define HYBRIS_WRAPPER_1_6(ret,arg0,arg1,arg2,arg3,arg4,arg5,sym)	\
  ret sym(arg0 _arg0, arg1 _arg1, arg2 _arg2, arg3 _arg3, arg4 _arg4, arg5 _arg5) { \
    static ret (* _sym)(arg0, arg1, arg2, arg3, arg4, arg5) = NULL;	\
    if (!_sym)								\
      _sym = __resolve_sym(#sym);					\
    return _sym(_arg0,_arg1, _arg2, _arg3, _arg4, _arg5);		\
  }									\

#define HYBRIS_WRAPPER_1_7(ret,arg0,arg1,arg2,arg3,arg4,arg5,arg6,sym)	\
  ret sym(arg0 _arg0, arg1 _arg1, arg2 _arg2, arg3 _arg3, arg4 _arg4, arg5 _arg5, arg6 _arg6) { \
    static ret (* _sym)(arg0, arg1, arg2, arg3, arg4, arg5, arg6) = NULL; \
    if (!_sym)								\
      _sym = __resolve_sym(#sym);					\
    return _sym(_arg0,_arg1, _arg2, _arg3, _arg4, _arg5, _arg6);	\
  }									\

#define HYBRIS_WRAPPER_0_4(arg0,arg1,arg2,arg3,sym)		     \
  void sym(arg0 _arg0, arg1 _arg1, arg2 _arg2, arg3 _arg3) {	     \
    static void (* _sym)(arg0, arg1, arg2, arg3) = NULL;	     \
    if (!_sym)							     \
      _sym = __resolve_sym(#sym);				     \
    _sym(_arg0,_arg1, _arg2, _arg3);				     \
  }								     \

#define HYBRIS_WRAPPER_0_1(arg0,sym)				     \
  void sym(arg0 _arg0) {					     \
    static void (* _sym)(arg0) = NULL;				     \
    if (!_sym)							     \
      _sym = __resolve_sym(#sym);				     \
    _sym(_arg0);						     \
  }								     \

#define HYBRIS_WRAPPER_0_2(arg0,arg1,sym)				\
  void sym(arg0 _arg0, arg1 _arg1) {					\
    static void (* _sym)(arg0, arg1) = NULL;				\
    if (!_sym)								\
      _sym = __resolve_sym(#sym);					\
    _sym(_arg0, _arg1);							\
  }									\

#define HYBRIS_WRAPPER_0_0(sym)						\
  void sym() {								\
    static void (* _sym)() = NULL;					\
    if (!_sym)								\
      _sym = __resolve_sym(#sym);					\
    _sym();								\
  }									\

#define HYBRIS_WRAPPER_0_3(arg0,arg1,arg2,sym)			     \
  void sym(arg0 _arg0, arg1 _arg1, arg2 _arg2) {		     \
    static void (* _sym)(arg0, arg1, arg2) = NULL;		     \
    if (!_sym)							     \
      _sym = __resolve_sym(#sym);				     \
    _sym(_arg0,_arg1, _arg2);					     \
  }								     \

HYBRIS_WRAPPER_1_0(int,droid_media_camera_get_number_of_cameras)
HYBRIS_WRAPPER_1_2(bool,DroidMediaCameraInfo*, int, droid_media_camera_get_info)
HYBRIS_WRAPPER_1_1(DroidMediaCamera*,int,droid_media_camera_connect)
HYBRIS_WRAPPER_1_1(bool,DroidMediaCamera*,droid_media_camera_reconnect)
HYBRIS_WRAPPER_0_1(DroidMediaCamera*,droid_media_camera_disconnect)
HYBRIS_WRAPPER_1_1(bool,DroidMediaCamera*,droid_media_camera_lock)
HYBRIS_WRAPPER_1_1(bool,DroidMediaCamera*,droid_media_camera_unlock)
HYBRIS_WRAPPER_1_1(bool,DroidMediaCamera*,droid_media_camera_start_preview)
HYBRIS_WRAPPER_0_1(DroidMediaCamera*,droid_media_camera_stop_preview)
HYBRIS_WRAPPER_1_1(bool,DroidMediaCamera*,droid_media_camera_is_preview_enabled)
HYBRIS_WRAPPER_1_1(bool,DroidMediaCamera*,droid_media_camera_start_recording)
HYBRIS_WRAPPER_0_1(DroidMediaCamera*,droid_media_camera_stop_recording)
HYBRIS_WRAPPER_1_1(bool,DroidMediaCamera*,droid_media_camera_is_recording_enabled)
HYBRIS_WRAPPER_1_1(bool,DroidMediaCamera*,droid_media_camera_start_auto_focus)
HYBRIS_WRAPPER_1_1(bool,DroidMediaCamera*,droid_media_camera_cancel_auto_focus)
HYBRIS_WRAPPER_0_3(DroidMediaCamera*,DroidMediaCameraCallbacks*,void*,droid_media_camera_set_callbacks)
HYBRIS_WRAPPER_1_4(bool,DroidMediaCamera*,int32_t,int32_t,int32_t,droid_media_camera_send_command)
HYBRIS_WRAPPER_1_2(bool, DroidMediaCamera*,bool,droid_media_camera_store_meta_data_in_buffers)
HYBRIS_WRAPPER_0_2(DroidMediaCamera*, int, droid_media_camera_set_preview_callback_flags)
HYBRIS_WRAPPER_1_2(bool, DroidMediaCamera*,const char *,droid_media_camera_set_parameters)
HYBRIS_WRAPPER_1_1(char *, DroidMediaCamera*, droid_media_camera_get_parameters)
HYBRIS_WRAPPER_1_2(bool, DroidMediaCamera*, int, droid_media_camera_take_picture)
HYBRIS_WRAPPER_1_1(DroidMediaBufferQueue*, DroidMediaCamera*, droid_media_camera_get_buffer_queue)
HYBRIS_WRAPPER_0_3(DroidMediaBuffer*,EGLDisplay,EGLSyncKHR,droid_media_buffer_release)
HYBRIS_WRAPPER_0_2(DroidMediaCamera*, DroidMediaCameraRecordingData*,droid_media_camera_release_recording_frame)
HYBRIS_WRAPPER_1_1(nsecs_t,DroidMediaCameraRecordingData*,droid_media_camera_recording_frame_get_timestamp)
HYBRIS_WRAPPER_1_1(size_t,DroidMediaCameraRecordingData*,droid_media_camera_recording_frame_get_size)
HYBRIS_WRAPPER_1_1(void*,DroidMediaCameraRecordingData*,droid_media_camera_recording_frame_get_data)
HYBRIS_WRAPPER_1_3(bool,DroidMediaCamera*,DroidMediaCameraFaceDetectionType,bool,droid_media_camera_enable_face_detection)
HYBRIS_WRAPPER_1_7(DroidMediaBuffer*,uint32_t,uint32_t,uint32_t,uint32_t,uint32_t,DroidMediaData*,DroidMediaBufferCallbacks*,droid_media_buffer_create_from_raw_data);
HYBRIS_WRAPPER_0_2(DroidMediaBuffer*,DroidMediaBufferInfo*,droid_media_buffer_get_info)
HYBRIS_WRAPPER_1_1(uint32_t,DroidMediaBuffer*,droid_media_buffer_get_transform)
HYBRIS_WRAPPER_1_1(uint32_t,DroidMediaBuffer*,droid_media_buffer_get_scaling_mode)
HYBRIS_WRAPPER_1_1(int64_t,DroidMediaBuffer*,droid_media_buffer_get_timestamp)
HYBRIS_WRAPPER_1_1(uint64_t,DroidMediaBuffer*,droid_media_buffer_get_frame_number)
HYBRIS_WRAPPER_1_1(DroidMediaRect,DroidMediaBuffer*,droid_media_buffer_get_crop_rect)
HYBRIS_WRAPPER_1_1(uint32_t,DroidMediaBuffer*,droid_media_buffer_get_width);
HYBRIS_WRAPPER_1_1(uint32_t,DroidMediaBuffer*,droid_media_buffer_get_height);
HYBRIS_WRAPPER_1_1(DroidMediaCodec*,DroidMediaCodecDecoderMetaData*,droid_media_codec_create_decoder);
HYBRIS_WRAPPER_1_1(DroidMediaCodec*,DroidMediaCodecEncoderMetaData*,droid_media_codec_create_encoder);
HYBRIS_WRAPPER_1_1(bool,DroidMediaCodec*,droid_media_codec_start);
HYBRIS_WRAPPER_0_1(DroidMediaCodec*,droid_media_codec_stop);
HYBRIS_WRAPPER_0_1(DroidMediaCodec *,droid_media_codec_destroy);
HYBRIS_WRAPPER_0_3(DroidMediaCodec*,DroidMediaCodecData*,DroidMediaBufferCallbacks*,droid_media_codec_queue);
HYBRIS_WRAPPER_1_1(DroidMediaBufferQueue*,DroidMediaCodec*,droid_media_codec_get_buffer_queue);
HYBRIS_WRAPPER_0_3(DroidMediaCodec*,DroidMediaCodecCallbacks*, void*, droid_media_codec_set_callbacks);
HYBRIS_WRAPPER_0_3(DroidMediaCodec*,DroidMediaCodecDataCallbacks*,void*,droid_media_codec_set_data_callbacks);

HYBRIS_WRAPPER_0_1(DroidMediaCodec *, droid_media_codec_flush);
HYBRIS_WRAPPER_0_1(DroidMediaCodec *, droid_media_codec_drain);
HYBRIS_WRAPPER_1_1(DroidMediaCodecLoopReturn,DroidMediaCodec*,droid_media_codec_loop);
HYBRIS_WRAPPER_0_3(DroidMediaCodec*,DroidMediaCodecMetaData*,DroidMediaRect*,droid_media_codec_get_output_info);
HYBRIS_WRAPPER_0_0(droid_media_init)
HYBRIS_WRAPPER_0_0(droid_media_deinit)
HYBRIS_WRAPPER_1_2(DroidMediaBuffer*,DroidMediaBufferQueue*,DroidMediaBufferCallbacks*,droid_media_buffer_queue_acquire_buffer)
HYBRIS_WRAPPER_0_3(DroidMediaBufferQueue*,DroidMediaBufferQueueCallbacks*,void*,droid_media_buffer_queue_set_callbacks)
HYBRIS_WRAPPER_1_2(bool,DroidMediaBufferQueue*,DroidMediaBufferInfo*,droid_media_buffer_queue_acquire_and_release)
HYBRIS_WRAPPER_0_1(DroidMediaCameraConstants*,droid_media_camera_constants_init)
HYBRIS_WRAPPER_0_1(DroidMediaPixelFormatConstants*,droid_media_pixel_format_constants_init)
HYBRIS_WRAPPER_1_1(int32_t,DroidMediaCamera*,droid_media_camera_get_video_color_format)
HYBRIS_WRAPPER_1_0(DroidMediaConvert*,droid_media_convert_create);
HYBRIS_WRAPPER_0_1(DroidMediaConvert*,droid_media_convert_destroy);
HYBRIS_WRAPPER_1_3(bool,DroidMediaConvert*,DroidMediaData*,void*,droid_media_convert_to_i420);
HYBRIS_WRAPPER_0_4(DroidMediaConvert*,DroidMediaRect,int32_t,int32_t,droid_media_convert_set_crop_rect);
HYBRIS_WRAPPER_1_1(bool,DroidMediaConvert*,droid_media_convert_is_i420);
