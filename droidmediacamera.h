#ifndef DROID_MEDIA_CAMERA_H
#define DROID_MEDIA_CAMERA_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#else
#include <stdbool.h>
  typedef void DroidMediaCamera;
#endif

struct DroidMediaCamera;
  /*
typedef struct {
  void *data;
  size_t size;
} DroidMediaCameraMemory;
  */
typedef struct {
  int facing;
  int orientation;
} DroidMediaCameraInfo;

typedef struct {
  void *data;
  void (* notify)(void *data, int32_t msgType, int32_t ext1, int32_t ext2);
  /*
// TODO:
    virtual void postData(int32_t msgType, const sp<IMemory>& dataPtr,                                             
                          camera_frame_metadata_t *metadata) = 0;                                                  
    virtual void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const sp<IMemory>& dataPtr) = 0;            

   */
} DroidMediaCameraCallbacks;

void droid_media_camera_init();
void droid_media_camera_deinit();

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

void droid_media_camera_set_callbacks(DroidMediaCamera *camera, DroidMediaCameraCallbacks *cb);
bool droid_media_camera_send_command(DroidMediaCamera *camera, int32_t cmd, int32_t arg1, int32_t arg2);
bool droid_media_camera_store_meta_data_in_buffers(DroidMediaCamera *camera, bool enabled);
void droid_media_camera_set_preview_callback_flags(DroidMediaCamera *camera, int preview_callback_flag);

bool droid_media_camera_set_parameters(DroidMediaCamera *camera, const char *params);
char *droid_media_camera_get_parameters(DroidMediaCamera *camera);

bool droid_media_camera_take_picture(DroidMediaCamera *camera, int msgType);

  // TODO: release recording frame

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_CAMERA_H */
