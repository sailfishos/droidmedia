//#define protected public // Slap me for that

#include "droidmediacamera.h"
#include "allocator.h"
#include <camera/Camera.h>
#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <gui/SurfaceTexture.h>
#include <android/log.h>
#include <utils/String8.h>
#include "mediabuffer.h"

extern "C" {

class DroidMediaCameraRecordingData
{
public:
    android::sp<android::IMemory> mem;
    nsecs_t ts;
};

class DroidMediaCamera
{
public:
    DroidMediaCamera() :
        m_cb(0),
        m_cb_data(0) {

    }

    android::sp<android::Camera> m_camera;
    android::sp<android::BufferQueue> m_queue;
    DroidMediaCameraCallbacks *m_cb;
    void *m_cb_data;
};

class BufferQueueListener : public android::BufferQueue::ConsumerListener {
public:
    BufferQueueListener() :
        m_cam(0)
    {

    }

    void onFrameAvailable()
    {
        if (m_cam->m_cb && m_cam->m_cb->frame_available) {
            m_cam->m_cb->frame_available(m_cam->m_cb_data);
        }
    }

    void onBuffersReleased()
    {
        if (m_cam->m_cb && m_cam->m_cb->buffers_released) {
            m_cam->m_cb->buffers_released(m_cam->m_cb_data);
        }
    }

    void setCamera(DroidMediaCamera *cam) {
        m_cam = cam;
    }

private:
    DroidMediaCamera *m_cam;
};

class CameraListener : public android::CameraListener {
public:
    CameraListener(DroidMediaCamera *cam) :
        m_cam(cam) {

    }

    void notify(int32_t msgType, int32_t ext1, int32_t ext2)
    {
        if (m_cam->m_cb && m_cam->m_cb->notify) {
            m_cam->m_cb->notify(m_cam->m_cb_data, msgType, ext1, ext2);
        }
    }

    void postData(int32_t msgType, const android::sp<android::IMemory>& dataPtr,
                  camera_frame_metadata_t *metadata)
    {
        DroidMediaData mem;
        mem.size = dataPtr->size();
        mem.data = dataPtr->pointer();

        if (m_cam->m_cb && m_cam->m_cb->post_data) {
            m_cam->m_cb->post_data(m_cam->m_cb_data, msgType, &mem);
        }

        // TODO: expose camera_frame_metadata_t
    }

    void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const android::sp<android::IMemory>& dataPtr)
    {
        if (m_cam->m_cb && m_cam->m_cb->post_data_timestamp) {
            DroidMediaCameraRecordingData *data = new DroidMediaCameraRecordingData;
            data->mem = dataPtr;
            data->ts = timestamp;
            m_cam->m_cb->post_data_timestamp(m_cam->m_cb_data, msgType, data);
        }
    }

private:
    DroidMediaCamera *m_cam;
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

    android::sp<BufferQueueListener> listener = new BufferQueueListener;

    if (queue->consumerConnect(listener) != android::NO_ERROR) {
        ALOGE("Failed to set buffer consumer");
        return NULL;
    }

    DroidMediaCamera *cam = new DroidMediaCamera;
    if (!cam) {
        ALOGE("Failed to allocate DroidMediaCamera");
        return NULL;
    }

    listener->setCamera(cam);

    cam->m_camera = android::Camera::connect(camera_number);
    if (cam->m_camera.get() == NULL) {
        delete cam;
        ALOGE("Failed to connect to camera service");
        return NULL;
    }

    cam->m_queue = queue;

    cam->m_camera->setPreviewTexture(cam->m_queue);

    cam->m_camera->setListener(new CameraListener(cam));

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

bool droid_media_camera_start_auto_focus(DroidMediaCamera *camera)
{
    return camera->m_camera->autoFocus() == android::NO_ERROR;
}

bool droid_media_camera_cancel_auto_focus(DroidMediaCamera *camera)
{
    return camera->m_camera->cancelAutoFocus() == android::NO_ERROR;
}

void droid_media_camera_set_callbacks(DroidMediaCamera *camera, DroidMediaCameraCallbacks *cb, void *data)
{
    camera->m_cb = cb;
    camera->m_cb_data = data;
}

bool droid_media_camera_send_command(DroidMediaCamera *camera, int32_t cmd, int32_t arg1, int32_t arg2)
{
    return camera->m_camera->sendCommand(cmd, arg1, arg2) == android::NO_ERROR;
}

bool droid_media_camera_store_meta_data_in_buffers(DroidMediaCamera *camera, bool enabled)
{
    return camera->m_camera->storeMetaDataInBuffers(enabled) == android::NO_ERROR;
}

void droid_media_camera_set_preview_callback_flags(DroidMediaCamera *camera, int preview_callback_flag)
{
    camera->m_camera->setPreviewCallbackFlags(preview_callback_flag);
}

bool droid_media_camera_set_parameters(DroidMediaCamera *camera, const char *params)
{
    return camera->m_camera->setParameters(android::String8(params)) == android::NO_ERROR;
}

char *droid_media_camera_get_parameters(DroidMediaCamera *camera)
{
    android::String8 p = camera->m_camera->getParameters();
    if (p.isEmpty()) {
        ALOGE("Failed to get camera parameters");
        return NULL;
    }

    size_t len = p.length();

    char *params = (char *)malloc(len + 1);
    if (!params) {
        ALOGE("Failed to allocate enough memory for camera parameters");
        return NULL;
    }

    memcpy(params, p.string(), len);
    params[len] = '\0';

    return params;
}

bool droid_media_camera_take_picture(DroidMediaCamera *camera, int msgType)
{
    return camera->m_camera->takePicture(msgType) == android::NO_ERROR;
}

DroidMediaBuffer *droid_media_camera_acquire_buffer(DroidMediaCamera *camera, DroidMediaBufferCallbacks *cb)
{
    android::BufferQueue::BufferItem buffer;
    int err = camera->m_queue->acquireBuffer(&buffer);
    if (err != android::OK) {
        ALOGE("DroidMediaCamera: Failed to acquire buffer from the queue. Error 0x%x", -err);
        return NULL;
    }

    if (!buffer.mGraphicBuffer.get()) {
        ALOGE("Got a buffer without real data");
        camera->m_queue->releaseBuffer(buffer.mBuf, EGL_NO_DISPLAY, EGL_NO_SYNC_KHR);
        return NULL;
    }

    return new DroidMediaBuffer(buffer, cb->data, cb->ref, cb->unref);
}

void droid_media_camera_release_buffer(DroidMediaCamera *camera, DroidMediaBuffer *buffer,
				       EGLDisplay display, EGLSyncKHR fence)
{
    android::BufferQueue::BufferItem buff = buffer->m_buffer;

    if (camera->m_queue->releaseBuffer(buff.mBuf, display, fence) != android::OK) {
        ALOGE("Failed to release preview buffer");
    }

    delete buffer;
}

void droid_media_camera_release_recording_frame(DroidMediaCamera *camera, DroidMediaCameraRecordingData *data)
{
    camera->m_camera->releaseRecordingFrame(data->mem);
    delete data;
}

nsecs_t droid_media_camera_recording_frame_get_timestamp(DroidMediaCameraRecordingData *data)
{
    return data->ts;
}

size_t droid_media_camera_recording_frame_get_size(DroidMediaCameraRecordingData *data)
{
    return data->mem->size();
}

void *droid_media_camera_recording_frame_get_data(DroidMediaCameraRecordingData *data)
{
    return data->mem->pointer();
}
};
