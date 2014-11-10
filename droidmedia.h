#ifndef DROID_MEDIA_H
#define DROID_MEDIA_H

#ifdef __cplusplus
extern "C" {
#endif

#include <sys/types.h>

typedef struct {
  void *data;
  size_t size;
} DroidMediaData;

void droid_media_init();
void droid_media_deinit();

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_H */
