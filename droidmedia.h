#ifndef DROID_MEDIA_H
#define DROID_MEDIA_H

#include <sys/types.h>

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

void droid_media_init();
void droid_media_deinit();

void droid_media_buffer_release(DroidMediaBuffer *buffer,
				EGLDisplay display, EGLSyncKHR fence);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_H */
