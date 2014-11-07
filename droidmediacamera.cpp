//#define protected public // Slap me for that

#include "droidmediacamera.h"
#include "allocator.h"
#include <camera/Camera.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <android/log.h>

extern "C" {

class DroidMediaCamera
{
public:
    android::sp<android::Camera> m_camera;
    android::sp<android::BufferQueue> m_queue;
#if 0
    android::sp<android::Surface> m_surface;
#endif
};

// class DroidMediaSurface : public android::Surface {
// public:
//     DroidMediaSurface(const android::sp<android::ISurfaceTexture>& st) :
//         android::Surface(android::sp<android::ISurfaceTexture>()) {

//     }

//     ~DroidMediaSurface() {
//     }
// };


class Listener : public android::BufferQueue::ConsumerListener {
public:
    void onFrameAvailable() {
        fprintf(stderr, "%s\n", __FUNCTION__);
    }

    void onBuffersReleased() {
        fprintf(stderr, "%s\n", __FUNCTION__);
    }
};

int droid_media_camera_get_number_of_cameras()
{
    return android::Camera::getNumberOfCameras();
}

bool droid_media_camera_get_info(DroidMediaCameraInfo *info, int camera_number)
{
    android::CameraInfo inf;

    if (android::Camera::getCameraInfo(camera_number, &inf) != 0) {
        return false;
    }

    info->orientation = inf.orientation;
    info->facing = inf.facing;

    return true;
}

DroidMediaCamera *droid_media_camera_connect(int camera_number)
{
    if (camera_number < 0 || camera_number >= droid_media_camera_get_number_of_cameras()) {
        ALOGE("incorrect camera number %d", camera_number);
        return NULL;
    }

//    android::sp<android::SurfaceComposerClient> surfaceClient(new android::SurfaceComposerClient);

//    android::sp<android::SurfaceControl> surfaceControl =
//        surfaceClient->createSurface(android::String8("camera"), 640, 480, android::PIXEL_FORMAT_RGBA_8888, 0);

//    android::sp<android::SurfaceControl> surfaceControl(new android::SurfaceControl);

    android::sp<android::BufferQueue>
        queue(new android::BufferQueue(new DroidMediaAllocator, true,
                                       android::BufferQueue::MIN_UNDEQUEUED_BUFFERS));
    if (!queue.get()) {
        ALOGE("Failed to get buffer queue");
        return NULL;
    }

    if (queue->consumerConnect(new Listener()) != android::NO_ERROR) {
        ALOGE("Failed to set buffer consumer");
        return NULL;
    }

#if 0
    android::ISurfaceTexture *sf = queue.get();
    if (!sf) {
        ALOGE("Failed to get surface texture");
        return NULL;
    }
#endif
#if 0
    android::sp<android::Surface> surface(new android::Surface(android::sp<android::ISurfaceTexture>(sf)));
#endif
//    android::sp<android::Surface> surface(new android::Surface(android::sp<android::ISurfaceTexture>()));
//    surface->setISurfaceTexture(sf);
#if 0
    if (!surface.get()) {
        ALOGE("Failed to get surface");
        return NULL;
    }
#endif
    DroidMediaCamera *cam = new DroidMediaCamera;
    if (!cam) {
        ALOGE("Failed to allocate DroidMediaCamera");
        return NULL;
    }

    cam->m_camera = android::Camera::connect(camera_number);
    if (cam->m_camera.get() == NULL) {
        delete cam;
        ALOGE("Failed to connect to camera service");
        return NULL;
    }

    cam->m_queue = queue;
#if 0
    cam->m_surface = surface;
#endif
    cam->m_camera->setPreviewTexture(cam->m_queue);

    return cam;
}

bool droid_media_camera_reconnect(DroidMediaCamera *camera) {
    return camera->m_camera->reconnect() == android::NO_ERROR;
}

void droid_media_camera_disconnect(DroidMediaCamera *camera)
{
    camera->m_camera->disconnect();

    delete camera;
}

bool droid_media_camera_lock(DroidMediaCamera *camera) {
    return camera->m_camera->lock() == android::NO_ERROR;
}

bool droid_media_camera_unlock(DroidMediaCamera *camera) {
    return camera->m_camera->unlock() == android::NO_ERROR;
}

bool droid_media_camera_start_preview(DroidMediaCamera *camera)
{
    return camera->m_camera->startPreview() == android::NO_ERROR;
}

void droid_media_camera_stop_preview(DroidMediaCamera *camera)
{
    camera->m_camera->stopPreview();
}

bool droid_media_camera_is_preview_enabled(DroidMediaCamera *camera)
{
    return camera->m_camera->previewEnabled();
}

bool droid_media_camera_start_recording(DroidMediaCamera *camera)
{
    return camera->m_camera->startRecording() == android::NO_ERROR;
}

void droid_media_camera_stop_recording(DroidMediaCamera *camera)
{
    camera->m_camera->stopRecording();
}

bool droid_media_camera_is_recording_enabled(DroidMediaCamera *camera)
{
    return camera->m_camera->recordingEnabled();
}

};
