#ifndef DROID_MEDIA_H
#define DROID_MEDIA_H

#include <sys/types.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
class DroidMediaBuffer;
#else
typedef void DroidMediaBuffer;
#endif

typedef void *EGLDisplay;
typedef void *EGLSyncKHR;

typedef struct {
  void *data;
  size_t size;
} DroidMediaData;

typedef struct {
  void (* ref)(void *data);
  void (* unref)(void *data);
  void *data;
} DroidMediaBufferCallbacks;

typedef struct {
  int32_t left;
  int32_t top;
  int32_t right;
  int32_t bottom;
} DroidMediaRect;

void droid_media_init();
void droid_media_deinit();

uint32_t droid_media_buffer_get_transform(DroidMediaBuffer * buffer);
uint32_t droid_media_buffer_get_scaling_mode(DroidMediaBuffer * buffer);
int64_t droid_media_buffer_get_timestamp(DroidMediaBuffer * buffer);
uint64_t droid_media_buffer_get_frame_number(DroidMediaBuffer * buffer);
DroidMediaRect droid_media_buffer_get_crop_rect(DroidMediaBuffer * buffer);
uint32_t droid_media_buffer_get_width(DroidMediaBuffer * buffer);
uint32_t droid_media_buffer_get_height(DroidMediaBuffer * buffer);
void droid_media_buffer_release(DroidMediaBuffer *buffer,
				EGLDisplay display, EGLSyncKHR fence);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_H */
