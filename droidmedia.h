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

#ifndef DROID_MEDIA_H
#define DROID_MEDIA_H

#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define DM_UNUSED __attribute__((unused))
#define DROID_MEDIA_EXPORT __attribute__ ((visibility ("default")))

typedef struct _DroidMediaBuffer DroidMediaBuffer;
typedef struct _DroidMediaBufferQueue DroidMediaBufferQueue;

typedef void *EGLDisplay;
typedef void *EGLSyncKHR;

typedef void (*DroidMediaCallback)(void *data);

typedef struct {
  void *data;
  ssize_t size;
} DroidMediaData;

typedef struct {
  DroidMediaCallback ref;
  DroidMediaCallback unref;
  void *data;
} DroidMediaBufferCallbacks;

typedef struct {
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;
} DroidMediaRect;

typedef struct {
  bool (* buffer_created)(void *data, DroidMediaBuffer *buffer);
  bool (* frame_available)(void *data, DroidMediaBuffer *buffer);
  void (* buffers_released)(void *data);
} DroidMediaBufferQueueCallbacks;

typedef struct {
  uint32_t transform;
  uint32_t scaling_mode;
  int64_t timestamp;
  uint64_t frame_number;
  DroidMediaRect crop_rect;
  uint32_t width;
  uint32_t height;
  int32_t format;
  int32_t stride;
} DroidMediaBufferInfo;

typedef enum {
    DROID_MEDIA_BUFFER_LOCK_READ = 0x01,
    DROID_MEDIA_BUFFER_LOCK_WRITE = 0x02,
    DROID_MEDIA_BUFFER_LOCK_READ_WRITE = 0x3
} DroidMediaBufferLockFlags;

/* droidmedia.cpp */
DROID_MEDIA_EXPORT void droid_media_init();
DROID_MEDIA_EXPORT void droid_media_deinit();

/* droidmediabuffer.cpp */
DROID_MEDIA_EXPORT DroidMediaBuffer *droid_media_buffer_create(uint32_t w, uint32_t h,
                                                               uint32_t format);
DROID_MEDIA_EXPORT void *droid_media_buffer_lock(DroidMediaBuffer *buffer, uint32_t flags);
DROID_MEDIA_EXPORT void droid_media_buffer_unlock(DroidMediaBuffer *buffer);

DROID_MEDIA_EXPORT void droid_media_buffer_get_info(DroidMediaBuffer *buffer, DroidMediaBufferInfo *info);
DROID_MEDIA_EXPORT uint32_t droid_media_buffer_get_transform(DroidMediaBuffer * buffer);
DROID_MEDIA_EXPORT uint32_t droid_media_buffer_get_scaling_mode(DroidMediaBuffer * buffer);
DROID_MEDIA_EXPORT int64_t droid_media_buffer_get_timestamp(DroidMediaBuffer * buffer);
DROID_MEDIA_EXPORT uint64_t droid_media_buffer_get_frame_number(DroidMediaBuffer * buffer);
DROID_MEDIA_EXPORT DroidMediaRect droid_media_buffer_get_crop_rect(DroidMediaBuffer * buffer);
DROID_MEDIA_EXPORT uint32_t droid_media_buffer_get_width(DroidMediaBuffer * buffer);
DROID_MEDIA_EXPORT uint32_t droid_media_buffer_get_height(DroidMediaBuffer * buffer);
DROID_MEDIA_EXPORT const void *droid_media_buffer_get_handle(DroidMediaBuffer *buffer);
DROID_MEDIA_EXPORT void droid_media_buffer_release(DroidMediaBuffer *buffer,
                                                   EGLDisplay display, EGLSyncKHR fence);
DROID_MEDIA_EXPORT void droid_media_buffer_destroy(DroidMediaBuffer *buffer);
DROID_MEDIA_EXPORT void droid_media_buffer_set_user_data(DroidMediaBuffer *buffer, void *data);
DROID_MEDIA_EXPORT void *droid_media_buffer_get_user_data(DroidMediaBuffer *buffer);

/* private.h */
DROID_MEDIA_EXPORT void droid_media_buffer_queue_set_callbacks(DroidMediaBufferQueue *queue,
                                                               DroidMediaBufferQueueCallbacks *cb, void *data);
DROID_MEDIA_EXPORT int droid_media_buffer_queue_length();

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_H */
