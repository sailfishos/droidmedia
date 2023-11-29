/*
 * Copyright (C) 2014-2015 Jolla Ltd.
 * Copyright (C) 2021 Open Mobile Platform LLC.
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
#if ANDROID_MAJOR < 8
#include "allocator.h"
#endif
#include <camera/Camera.h>
#include <camera/CameraParameters.h>
#include <android/log.h>
#include <utils/String8.h>
#include <utils/Condition.h>
#if ANDROID_MAJOR >= 8
#include <media/hardware/MetadataBufferType.h>
#endif
#include <media/stagefright/CameraSource.h>
#include <media/openmax/OMX_IVCommon.h>
#include "droidmediabuffer.h"
#include "private.h"

#undef LOG_TAG
#define LOG_TAG "DroidMediaCamera"

namespace android {
	int32_t getColorFormat(const char* colorFormat) {
		if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV420P)) {
		   return OMX_COLOR_FormatYUV420Planar;
		}

		if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV422SP)) {
		   return OMX_COLOR_FormatYUV422SemiPlanar;
		}

		if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV420SP)) {
			return OMX_COLOR_FormatYUV420SemiPlanar;
		}

		if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_YUV422I)) {
			return OMX_COLOR_FormatYCbYCr;
		}

		if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_RGB565)) {
		   return OMX_COLOR_Format16bitRGB565;
		}

		if (!strcmp(colorFormat, "OMX_TI_COLOR_FormatYUV420PackedSemiPlanar")) {
		   return OMX_TI_COLOR_FormatYUV420PackedSemiPlanar;
		}
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR >= 2) || ANDROID_MAJOR >= 5
		if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_ANDROID_OPAQUE)) {
			return OMX_COLOR_FormatAndroidOpaque;
		}
#endif
		return -1;
	}
}

extern "C" {

struct _DroidMediaCameraRecordingData
{
    android::sp<android::IMemory> mem;
    nsecs_t ts;
};

struct _DroidMediaCamera
{
    _DroidMediaCamera() :
        m_cb_data(0) {
        memset(&m_cb, 0x0, sizeof(m_cb));
    }

    android::sp<android::Camera> m_camera;
    android::sp<DroidMediaBufferQueue> m_queue;
    android::sp<DroidMediaBufferQueue> m_recording_queue;
    DroidMediaCameraCallbacks m_cb;
    void *m_cb_data;
};

class CameraListener : public android::CameraListener {
public:
    CameraListener(DroidMediaCamera *cam) :
        m_cam(cam) {

    }

    void notify(int32_t msgType, int32_t ext1, int32_t ext2)
    {
        switch (msgType) {
            case CAMERA_MSG_SHUTTER:
                if (m_cam->m_cb.shutter_cb) {
                    m_cam->m_cb.shutter_cb(m_cam->m_cb_data);
                }
                break;

            case CAMERA_MSG_FOCUS:
                if (m_cam->m_cb.focus_cb) {
                    m_cam->m_cb.focus_cb(m_cam->m_cb_data, ext1);
                }
                break;

            case CAMERA_MSG_FOCUS_MOVE:
                if (m_cam->m_cb.focus_move_cb) {
                    m_cam->m_cb.focus_move_cb(m_cam->m_cb_data, ext1);
                }
                break;

            case CAMERA_MSG_ERROR:
                if (m_cam->m_cb.error_cb) {
                    m_cam->m_cb.error_cb(m_cam->m_cb_data, ext1);
                }
                break;

            case CAMERA_MSG_ZOOM:
                if (m_cam->m_cb.zoom_cb) {
                    m_cam->m_cb.zoom_cb(m_cam->m_cb_data, ext1, ext2);
                }
                break;
            default:
                ALOGW("unknown notify message 0x%x", msgType);
                break;
        }
    }

    void postData(int32_t msgType, const android::sp<android::IMemory>& dataPtr,
                  camera_frame_metadata_t *metadata)
    {
        int32_t dataMsgType = msgType & ~CAMERA_MSG_PREVIEW_METADATA;
        DroidMediaData mem;

        switch (dataMsgType) {
            case CAMERA_MSG_RAW_IMAGE:
                mem.size = dataPtr->size();
#if ANDROID_MAJOR >= 11
                mem.data = dataPtr->unsecurePointer();
#else
                mem.data = dataPtr->pointer();
#endif

                if (m_cam->m_cb.raw_image_cb) {
                    m_cam->m_cb.raw_image_cb(m_cam->m_cb_data, &mem);
                }
                break;

            case CAMERA_MSG_COMPRESSED_IMAGE:
                mem.size = dataPtr->size();
#if ANDROID_MAJOR >= 11
                mem.data = dataPtr->unsecurePointer();
#else
                mem.data = dataPtr->pointer();
#endif

                if (m_cam->m_cb.compressed_image_cb) {
                    m_cam->m_cb.compressed_image_cb(m_cam->m_cb_data, &mem);
                }
                break;

            case CAMERA_MSG_POSTVIEW_FRAME:
                mem.size = dataPtr->size();
#if ANDROID_MAJOR >= 11
                mem.data = dataPtr->unsecurePointer();
#else
                mem.data = dataPtr->pointer();
#endif
                if (m_cam->m_cb.postview_frame_cb) {
                    m_cam->m_cb.postview_frame_cb(m_cam->m_cb_data, &mem);
                }
                break;

            case CAMERA_MSG_RAW_IMAGE_NOTIFY:
                if (m_cam->m_cb.raw_image_notify_cb) {
                    m_cam->m_cb.raw_image_notify_cb(m_cam->m_cb_data);
                }
                break;

            case CAMERA_MSG_PREVIEW_FRAME:
                mem.size = dataPtr->size();
#if ANDROID_MAJOR >= 11
                mem.data = dataPtr->unsecurePointer();
#else
                mem.data = dataPtr->pointer();
#endif
                if (m_cam->m_cb.preview_frame_cb) {
                    m_cam->m_cb.preview_frame_cb(m_cam->m_cb_data, &mem);
                }
                break;

            case 0:
              // Nothing
              break;

            default:
                ALOGW("unknown postData message 0x%x", dataMsgType);
                break;
        }

        if (metadata && (msgType & CAMERA_MSG_PREVIEW_METADATA)
            && m_cam->m_cb.preview_metadata_cb) {
            sendPreviewMetadata(metadata);
        }
    }

    void postDataTimestamp(nsecs_t timestamp, int32_t msgType, const android::sp<android::IMemory>& dataPtr)
    {
        switch (msgType) {
            case CAMERA_MSG_VIDEO_FRAME:
                if (m_cam->m_cb.video_frame_cb) {
                    // TODO: revisit this data structure
                    DroidMediaCameraRecordingData *data = new DroidMediaCameraRecordingData;
                    data->mem = dataPtr;
                    data->ts = timestamp;
                    m_cam->m_cb.video_frame_cb(m_cam->m_cb_data, data);
                } else {
                    m_cam->m_camera->releaseRecordingFrame(dataPtr);
                }

                break;

            default:
                ALOGW("unknown postDataTimestamp message 0x%x", msgType);
                break;
        }
    }
#if ANDROID_MAJOR >= 7
    void postRecordingFrameHandleTimestamp(nsecs_t timestamp, native_handle_t* handle) 
    {
    	ALOGW("postRecordingFrameHandleTimestamp - not sure what to do");
    }
#endif

#if ANDROID_MAJOR >= 8
    void postRecordingFrameHandleTimestampBatch(const std::vector<nsecs_t>& timestamps,
                                                const std::vector<native_handle_t*>& handles)
    {
        ALOGW("postRecordingFrameHandleTimestampBatch - not sure what to do");
    }
#endif

    void sendPreviewMetadata(camera_frame_metadata_t *metadata)
    {
        android::Vector<DroidMediaCameraFace> faces;

        for (int x = 0; x < metadata->number_of_faces; x++) {
            DroidMediaCameraFace face;
            face.left = metadata->faces[x].rect[0];
            face.top = metadata->faces[x].rect[1];
            face.right = metadata->faces[x].rect[2];
            face.bottom = metadata->faces[x].rect[3];
            face.score = metadata->faces[x].score;
            face.id = metadata->faces[x].id;
            face.left_eye[0] = metadata->faces[x].left_eye[0];
            face.left_eye[1] = metadata->faces[x].left_eye[1];
            face.right_eye[0] = metadata->faces[x].right_eye[0];
            face.right_eye[1] = metadata->faces[x].right_eye[1];
            face.mouth[0] = metadata->faces[x].mouth[0];
            face.mouth[1] = metadata->faces[x].mouth[1];

            faces.push_back(face);
        }

        m_cam->m_cb.preview_metadata_cb(m_cam->m_cb_data, faces.array(), faces.size());
    }

private:
    DroidMediaCamera *m_cam;
};

DroidMediaBufferQueue *droid_media_camera_get_buffer_queue (DroidMediaCamera *camera)
{
  return camera->m_queue.get();
}

DroidMediaBufferQueue *droid_media_camera_get_recording_buffer_queue (DroidMediaCamera *camera)
{
  return camera->m_recording_queue.get();
}

int droid_media_camera_get_number_of_cameras()
{
    return android::Camera::getNumberOfCameras();
}

bool droid_media_camera_get_info(DroidMediaCameraInfo *info, int camera_number)
{
    android::CameraInfo inf;

    if (android::Camera::getCameraInfo(camera_number,
#if ANDROID_MAJOR >= 13 && (!defined(LEGACY_ANDROID_13_REVISION) || LEGACY_ANDROID_13_REVISION >= 32)
                                       false/*overrideToPortrait*/,
#endif
                                       &inf) != 0) {
        return false;
    }

    info->orientation = inf.orientation;
    if (inf.facing == CAMERA_FACING_FRONT) {
      info->facing = DROID_MEDIA_CAMERA_FACING_FRONT;
    } else {
      info->facing = DROID_MEDIA_CAMERA_FACING_BACK;
    }

    return true;
}

DroidMediaCamera *droid_media_camera_connect(int camera_number)
{
    android::sp<DroidMediaBufferQueue>
      queue(new DroidMediaBufferQueue("DroidMediaCameraBufferQueue"));
    if (!queue->connectListener()) {
        ALOGE("Failed to connect buffer queue listener");
        return NULL;
    }

    DroidMediaCamera *cam = new DroidMediaCamera;
    if (!cam) {
        ALOGE("Failed to allocate DroidMediaCamera");
        return NULL;
    }

#if (ANDROID_MAJOR == 4 && ANDROID_MINOR < 4)
    cam->m_camera = android::Camera::connect(camera_number);
#elif (ANDROID_MAJOR == 4 && ANDROID_MINOR == 4)
    cam->m_camera = android::Camera::connect(camera_number, android::String16("droidmedia"),
					     android::Camera::USE_CALLING_UID);
#else // Force HAL version if defined
#ifdef FORCE_HAL
    ALOGI("Connecting HAL version %d", FORCE_HAL);
    android::OK != android::Camera::connectLegacy(camera_number, FORCE_HAL << 8, android::String16("droidmedia"),
					     android::Camera::USE_CALLING_UID, cam->m_camera);
#else // Default connect
    cam->m_camera = android::Camera::connect(camera_number, android::String16("droidmedia"),
					     android::Camera::USE_CALLING_UID
#if (ANDROID_MAJOR >= 7)
					     , android::Camera::USE_CALLING_PID
#endif
#if (ANDROID_MAJOR >= 12)
					     , __ANDROID_API_FUTURE__
#endif
#if (ANDROID_MAJOR >= 13) && (!defined(LEGACY_ANDROID_13_REVISION) || LEGACY_ANDROID_13_REVISION >= 32)
					     , false
#endif
#if (ANDROID_MAJOR >= 13) && (!defined(LEGACY_ANDROID_13_REVISION) || LEGACY_ANDROID_13_REVISION >= 50)
					     , false
#endif
					      );
#endif
#endif
    if (cam->m_camera.get() == NULL) {
        delete cam;
        ALOGE("Failed to connect to camera service");
        return NULL;
    }

    cam->m_queue = queue;
    cam->m_queue->attachToCameraPreview(cam->m_camera);

#if ANDROID_MAJOR >= 9
    android::sp<DroidMediaBufferQueue>
      recording_queue(new DroidMediaBufferQueue("DroidMediaCameraBufferRecordingQueue"));
    if (!recording_queue->connectListener()) {
        ALOGE("Failed to connect video buffer queue listener");
    } else {
      cam->m_recording_queue = recording_queue;
      cam->m_recording_queue->attachToCameraVideo(cam->m_camera);
      cam->m_camera->setVideoBufferMode(
                      android::hardware::ICamera::VIDEO_BUFFER_MODE_BUFFER_QUEUE);
    }
#endif

    cam->m_camera->setListener(new CameraListener(cam));

    return cam;
}

bool droid_media_camera_reconnect(DroidMediaCamera *camera) {
    return camera->m_camera->reconnect() == android::NO_ERROR;
}

void droid_media_camera_disconnect(DroidMediaCamera *camera)
{
    camera->m_camera->disconnect();

    camera->m_queue->setCallbacks(0, 0);

#if ANDROID_MAJOR >= 9
    if (camera->m_recording_queue.get()) {
      camera->m_recording_queue->setCallbacks(0, 0);
    }
#endif

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
    memcpy(&camera->m_cb, cb, sizeof(camera->m_cb));
    camera->m_cb_data = data;
}

bool droid_media_camera_send_command(DroidMediaCamera *camera, int32_t cmd, int32_t arg1, int32_t arg2)
{
    return camera->m_camera->sendCommand(cmd, arg1, arg2) == android::NO_ERROR;
}

bool droid_media_camera_store_meta_data_in_buffers(DroidMediaCamera *camera, bool enabled)
{
#if ANDROID_MAJOR < 7
    return camera->m_camera->storeMetaDataInBuffers(enabled) == android::NO_ERROR;
#else
    if (enabled) {
        if (android::OK == camera->m_camera->setVideoBufferMode(
                android::hardware::ICamera::VIDEO_BUFFER_MODE_BUFFER_QUEUE)) {
            ALOGI("Recording in buffer queue mode");
            return true;
        } else if (android::OK == camera->m_camera->setVideoBufferMode(
                android::hardware::ICamera::VIDEO_BUFFER_MODE_DATA_CALLBACK_METADATA)) {
            ALOGI("Recording in callback metadata mode");
            return true;
        }
    }
    camera->m_camera->setVideoBufferMode(
                android::hardware::ICamera::VIDEO_BUFFER_MODE_DATA_CALLBACK_YUV);
    ALOGI("Recording in callback yuv mode");
    return !enabled; // false if metadata mode was requested.
#endif
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
#if ANDROID_MAJOR >= 11
    return data->mem->unsecurePointer();
#else
    return data->mem->pointer();
#endif
}

bool droid_media_camera_enable_face_detection(DroidMediaCamera *camera,
					      DroidMediaCameraFaceDetectionType type, bool enable)
{
  int detection_type = type == DROID_MEDIA_CAMERA_FACE_DETECTION_HW ? CAMERA_FACE_DETECTION_HW :
    CAMERA_FACE_DETECTION_SW;

  int cmd = enable ? CAMERA_CMD_START_FACE_DETECTION : CAMERA_CMD_STOP_FACE_DETECTION;

  return droid_media_camera_send_command (camera, cmd, detection_type, 0);
}

int32_t droid_media_camera_get_video_color_format (DroidMediaCamera *camera)
{

  android::CameraParameters p(camera->m_camera->getParameters());

  return android::getColorFormat(p.get(android::CameraParameters::KEY_VIDEO_FRAME_FORMAT));
}

};

android::sp<android::Camera> droid_media_camera_get_camera (DroidMediaCamera *camera) {
  return camera->m_camera;
}
