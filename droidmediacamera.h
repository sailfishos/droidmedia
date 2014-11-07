#ifndef DROID_MEDIA_CAMERA_H
#define DROID_MEDIA_CAMERA_H

extern "C" {
struct DroidMediaCamera;

  typedef struct {
    int facing;
    int orientation;
  } DroidMediaCameraInfo;

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
};

#endif /* DROID_MEDIA_CAMERA_H */
