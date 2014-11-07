//#define protected public // Slap me for that

#include "droidmediacamera.h"
#include "allocator.h"
#include <camera/Camera.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceTexture.h>
#include <android/log.h>
#include <utils/String8.h>
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>

extern "C" {

class DroidMediaCamera
{
public:
    android::sp<android::Camera> m_camera;
    android::sp<android::BufferQueue> m_queue;
    android::sp<android::BufferQueue::ConsumerListener> m_listener;
};

class Listener : public android::BufferQueue::ConsumerListener {
public:
    Listener() :
        m_cam(0)
    {

    }

    void onFrameAvailable()
    {
        fprintf(stderr, "%s\n", __FUNCTION__);
        android::BufferQueue::BufferItem buffer;
        if (m_cam->m_queue->acquireBuffer(&buffer) != android::OK) {
            ALOGE("DroidMediaCamera: Failed to acquire buffer from the queue");
        } else {
            m_cam->m_queue->releaseBuffer(buffer.mBuf, NULL, NULL);
        }
    }

    void onBuffersReleased()
    {
        fprintf(stderr, "%s\n", __FUNCTION__);
    }

    void setCamera(DroidMediaCamera *cam) {
        m_cam = cam;
    }

private:
    DroidMediaCamera *m_cam;
};

void droid_media_camera_init()
{
    android::ProcessState::self()->startThreadPool();
}

void droid_media_camera_deinit()
{
    android::IPCThreadState::self()->stopProcess(false);
    android::IPCThreadState::self()->joinThreadPool();
}

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

    android::sp<android::BufferQueue>
        queue(new android::BufferQueue(new DroidMediaAllocator, true,
                                       android::BufferQueue::MIN_UNDEQUEUED_BUFFERS));
    if (!queue.get()) {
        ALOGE("Failed to get buffer queue");
        return NULL;
    }

    queue->setConsumerName(android::String8("DroidMediaBufferQueue"));
    queue->setConsumerUsageBits(android::GraphicBuffer::USAGE_HW_TEXTURE);
    queue->setSynchronousMode(false);

    android::sp<android::BufferQueue::ConsumerListener> listener = new Listener;

    if (queue->consumerConnect(listener) != android::NO_ERROR) {
        ALOGE("Failed to set buffer consumer");
        return NULL;
    }

    DroidMediaCamera *cam = new DroidMediaCamera;
    if (!cam) {
        ALOGE("Failed to allocate DroidMediaCamera");
        return NULL;
    }

    static_cast<Listener *>(listener.get())->setCamera(cam);
    cam->m_listener = listener;

    cam->m_camera = android::Camera::connect(camera_number);
    if (cam->m_camera.get() == NULL) {
        delete cam;
        ALOGE("Failed to connect to camera service");
        return NULL;
    }

    cam->m_queue = queue;

    cam->m_camera->setPreviewTexture(cam->m_queue);

    cam->m_camera->sendCommand(CAMERA_CMD_START_FACE_DETECTION, 1, 1);

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
