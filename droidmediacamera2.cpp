/*
 * Copyright (C) 2023 Jolla Ltd.
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
 */

#if ANDROID_MAJOR >= 7
#include "droidmediacamera.h"

#include <camera/CameraParameters.h>
#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraError.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraMetadata.h>
#include <camera/NdkCameraMetadataTags.h>
#include <camera/NdkCaptureRequest.h>
#include <media/hardware/MetadataBufferType.h>
#include <media/NdkImage.h>
#include <media/NdkImageReader.h>
#include <media/openmax/OMX_IVCommon.h>
#include <media/stagefright/CameraSource.h>

#include <string>
#include <unordered_map>
#include <set>

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
        if (!strcmp(colorFormat, CameraParameters::PIXEL_FORMAT_ANDROID_OPAQUE)) {
            return OMX_COLOR_FormatAndroidOpaque;
        }
        return -1;
    }

    const char *getColorFormatString(int32_t colorFormat, bool &found) {
        found = true;
        switch (colorFormat) {
        case AIMAGE_FORMAT_YUV_420_888: {
            std::string out = CameraParameters::PIXEL_FORMAT_YUV420P;
            out += ",";
            out += CameraParameters::PIXEL_FORMAT_YUV420SP;
            const char *o = out.c_str();
            return o;
        }
        default:
            found = false;
            return "";
        }
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
        m_cb_data(NULL) {
        memset(&m_cb, 0x0, sizeof(m_cb));
    }

    ~_DroidMediaCamera() {
        if (m_manager) {
            ACameraManager_delete(m_manager);
            m_manager = NULL;
        }
    }

    ACameraDevice *m_device = NULL;
    ACameraIdList *m_camera_id_list = NULL;
    ACameraManager *m_manager = NULL;
    ACameraMetadata *m_metadata = NULL;

    // Callbacks
    ACameraDevice_StateCallbacks m_device_state_callbacks;
    ACameraCaptureSession_stateCallbacks m_capture_session_state_callbacks;
    ACameraCaptureSession_captureCallbacks m_capture_callbacks;

    // Capture session
    ACameraCaptureSession *m_session = NULL;
    ACaptureSessionOutputContainer *m_capture_session_output_container = NULL;

    // Requests
    ACaptureRequest *m_preview_request = NULL;
    ACaptureRequest *m_image_request = NULL;
    ACaptureRequest *m_video_request = NULL;

    // Preview
    ACameraOutputTarget *m_preview_output_target = NULL;
    ACaptureSessionOutput *m_preview_output = NULL;
    bool m_preview_enabled = false;

    // Image capture
    AImageReader *m_image_reader = NULL;
    AImageReader_ImageListener m_image_listener;
    ANativeWindow* m_image_reader_anw = NULL;
    ACaptureSessionOutput *m_image_reader_output = NULL;
    ACameraOutputTarget *m_image_reader_output_target = NULL;

    // Video recording
    ACameraOutputTarget *m_video_output_target = NULL;
    ACaptureSessionOutput *m_video_output = NULL;
    bool m_video_recording_enabled = false;

    // Queues
    android::sp<DroidMediaBufferQueue> m_queue;
    android::sp<DroidMediaBufferQueue> m_recording_queue;

    bool m_video_mode = false;

    int32_t image_format = AIMAGE_FORMAT_JPEG;
    int32_t image_height = -1;
    int32_t image_width = -1;
    int32_t preview_height = -1;
    int32_t preview_width = -1;
    int32_t video_height = -1;
    int32_t video_width = -1;

    int32_t max_ae_regions = 0;
    int32_t max_awb_regions = 0;
    int32_t max_focus_regions = 0;

    DroidMediaCameraCallbacks m_cb;
    void *m_cb_data;
};

static void still_image_available(void* context, AImageReader* reader)
{
    ALOGI("Still image available");
// TODO send image data
    AImage *image;
    media_status_t status;

    status = AImageReader_acquireNextImage(reader, &image);

    if (status == AMEDIA_OK) {
        DroidMediaCamera *camera = (DroidMediaCamera *)context;
        DroidMediaData mem;
        int32_t format;

        status = AImage_getFormat(image, &format);

        switch (format) {
        case AIMAGE_FORMAT_JPEG: {
            int32_t num_planes = 0;

            status = AImage_getNumberOfPlanes(image, &num_planes);
            if (status != AMEDIA_OK || num_planes != 1) {
                break;
            }

            status = AImage_getPlaneData(image, 0, (uint8_t **)&mem.data, (int *)&mem.size);
            if (status == AMEDIA_OK && camera->m_cb.compressed_image_cb) {
                camera->m_cb.compressed_image_cb(camera->m_cb_data, &mem);
            }
            break;
        }
/*
// TODO raw image
            if (m_cam->m_cb.raw_image_cb) {
                m_cam->m_cb.raw_image_cb(m_cam->m_cb_data, &mem);
            }
*/
        default:
            ALOGI("Unsupported image: %u", format);
            break;
        }
        AImage_delete(image);
    }
}

static void device_on_disconnected(void *context, ACameraDevice *device)
{
    ALOGI("Camera '%s' is diconnected.", ACameraDevice_getId(device));
}

static void device_on_error(void *context, ACameraDevice *device, int error)
{
    ALOGE("Error (%d) on Camera '%s'.", error, ACameraDevice_getId(device));
}

static void capture_session_on_active(void *context, ACameraCaptureSession *session)
{
    ALOGI("Session is activated. %p", session);
}

static void capture_session_on_closed(void *context, ACameraCaptureSession *session)
{
    ALOGI("Session is closed. %p", session);
}

static void capture_session_on_ready(void *context, ACameraCaptureSession *session)
{
    ALOGI("Session is ready. %p", session);
}

static void capture_session_on_capture_started(
    void* context, ACameraCaptureSession* session,
    const ACaptureRequest* request, int64_t timestamp)
{
    ACameraMetadata_const_entry entry;
    camera_status_t status = ACaptureRequest_getConstEntry(request, ACAMERA_CONTROL_CAPTURE_INTENT, &entry);

    if (status == ACAMERA_OK &&
            entry.data.u8[0] == ACAMERA_CONTROL_CAPTURE_INTENT_STILL_CAPTURE) {
        ALOGI("Calling shutter callback");
        DroidMediaCamera *camera = (DroidMediaCamera *)context;
        if (camera->m_cb.shutter_cb) {
            camera->m_cb.shutter_cb(camera->m_cb_data);
        }
    }
}

static void capture_session_on_capture_progressed(
    void* context, ACameraCaptureSession* session,
    ACaptureRequest* request, const ACameraMetadata* result)
{
    ALOGI("Capture progressed: %p", context);
    //TODO
}

static void capture_session_on_capture_completed(
    void *context, ACameraCaptureSession *session,
    ACaptureRequest *request, const ACameraMetadata *result)
{
    ALOGI("Capture completed: %p", context);
    ACameraMetadata_const_entry entry;

    camera_status_t status = ACameraMetadata_getConstEntry(result, ACAMERA_CONTROL_AF_TRIGGER, &entry);
                DroidMediaCamera *camera = (DroidMediaCamera *)context;
    if (status == ACAMERA_OK) {
        uint8_t value = entry.data.u8[0];
        ALOGI("AF trigger state: %i", value);
        if (value == ACAMERA_CONTROL_AF_TRIGGER_START) {
            ALOGI("AF trigger start found");
            uint8_t afTrigger = ACAMERA_CONTROL_AF_TRIGGER_IDLE;
            status = ACaptureRequest_setEntry_u8(camera->m_preview_request,
                ACAMERA_CONTROL_AF_TRIGGER, 1, &afTrigger);

            if (status == ACAMERA_OK) {
                ALOGI("AF state: restart preview start");
                status = ACameraCaptureSession_setRepeatingRequest(camera->m_session, &camera->m_capture_callbacks, 1,
                    &camera->m_preview_request, NULL);
            }
        } else if (value == ACAMERA_CONTROL_AF_TRIGGER_CANCEL) {
            ALOGI("AF trigger cancel found");
            uint8_t afTrigger = ACAMERA_CONTROL_AF_TRIGGER_IDLE;
                status = ACaptureRequest_setEntry_u8(camera->m_preview_request,
                ACAMERA_CONTROL_AF_TRIGGER, 1, &afTrigger);

            if (status == ACAMERA_OK) {
                ALOGI("AF state: restart preview cancel");
                status = ACameraCaptureSession_setRepeatingRequest(camera->m_session, &camera->m_capture_callbacks, 1,
                    &camera->m_preview_request, NULL);
            }
        }
    }

    status = ACameraMetadata_getConstEntry(result, ACAMERA_CONTROL_AF_STATE, &entry);
    if (status == ACAMERA_OK) {
        uint8_t value = entry.data.u8[0];
        int res = 0;
        ALOGI("AF state: %i", value);

        if (value == ACAMERA_CONTROL_AF_STATE_PASSIVE_FOCUSED ||
            value == ACAMERA_CONTROL_AF_STATE_FOCUSED_LOCKED) {
            res = 1;
        } else if (value == ACAMERA_CONTROL_AF_STATE_PASSIVE_UNFOCUSED ||
            value == ACAMERA_CONTROL_AF_STATE_NOT_FOCUSED_LOCKED) {
            res = 0;
        }
        ALOGI("AF state: %i", res);
        if (camera->m_cb.focus_cb && res >= 0) {
            camera->m_cb.focus_cb(camera->m_cb_data, res);
        }
    }
}

static void capture_session_on_capture_failed(
    void* context, ACameraCaptureSession* session,
    ACaptureRequest* request, ACameraCaptureFailure* failure)
{
    ALOGI("Capture failed: %p", context);
}

static void capture_session_on_capture_sequence_completed(
    void* context, ACameraCaptureSession* session,
    int sequenceId, int64_t frameNumber)
{
    ALOGI("Capture sequence completed: %p", context);
}

static void capture_session_on_capture_sequence_abort(
    void* context, ACameraCaptureSession* session, int sequenceId)
{
    ALOGI("Capture sequence aborted: %p", context);
}

static void capture_session_on_capture_buffer_lost(
    void* context, ACameraCaptureSession* session,
    ACaptureRequest* request, ACameraWindowType* window, int64_t frameNumber)
{
    ALOGI("Capture buffer lost: %p", context);
}

DroidMediaBufferQueue *droid_media_camera_get_buffer_queue (DroidMediaCamera *camera)
{
    ALOGI("get_buffer_queue");
    return camera->m_queue.get();
}

DroidMediaBufferQueue *droid_media_camera_get_recording_buffer_queue (DroidMediaCamera *camera)
{
    ALOGI("get_recording_buffer_queue");
    return camera->m_recording_queue.get();
}

int droid_media_camera_get_number_of_cameras()
{
    int num_cameras;
    camera_status_t status;
    ACameraIdList *camera_id_list = NULL;
    ACameraManager *camera_manager = ACameraManager_create();

    status = ACameraManager_getCameraIdList(camera_manager, &camera_id_list);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to get camera id list (status: %d)", status);
        return 0;
    }

    num_cameras = camera_id_list->numCameras;

    ACameraManager_deleteCameraIdList(camera_id_list);
    ACameraManager_delete(camera_manager);

    return num_cameras;
}

bool droid_media_camera_get_info(DroidMediaCameraInfo *info, int camera_number)
{
    const char *selected_camera_id = NULL;
    camera_status_t status;
    ACameraIdList *camera_id_list = NULL;
    ACameraMetadata *camera_metadata = NULL;
    ACameraMetadata_const_entry entry;
    ACameraManager *camera_manager = ACameraManager_create();
    size_t num_physical_cameras = 0;
    const char *const *physical_camera_ids = NULL;

    status = ACameraManager_getCameraIdList(camera_manager, &camera_id_list);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to get camera id list: %d", status);
        goto fail;
    }
    ALOGI("Get info from camera %i of %i", camera_number, camera_id_list->numCameras);

    if (camera_id_list->numCameras <= 0 ||
            camera_number >= camera_id_list->numCameras) {
        ALOGE("Invalid camera number");
        goto fail;
    }

    selected_camera_id = camera_id_list->cameraIds[camera_number];

    status = ACameraManager_getCameraCharacteristics(camera_manager,
                                                     selected_camera_id,
                                                     &camera_metadata);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to get camera characteristics for camera '%s': %d",
            camera_id_list->cameraIds[camera_number], status);
        goto fail;
    }

    if (ACameraMetadata_isLogicalMultiCamera(camera_metadata,
                                             &num_physical_cameras,
                                             &physical_camera_ids)) {
        ALOGI("Multicamera with physical camera count %zu", num_physical_cameras);
    }

    status = ACameraMetadata_getConstEntry(camera_metadata,
                                           ACAMERA_LENS_FACING,
                                           &entry);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to get camera lens facing: %d", status);
        goto fail;
    }

    if (entry.data.u8[0] == ACAMERA_LENS_FACING_FRONT) {
        info->facing = DROID_MEDIA_CAMERA_FACING_FRONT;
    } else {
        info->facing = DROID_MEDIA_CAMERA_FACING_BACK;
    }

    status = ACameraMetadata_getConstEntry(camera_metadata,
                                           ACAMERA_SENSOR_ORIENTATION,
                                           &entry);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to get camera orientation: %d", status);
        goto fail;
    }

    info->orientation = entry.data.i32[0];
    ALOGI("Camera %i facing %i orientation %i", camera_number, info->facing, info->orientation);

fail:
    if (camera_metadata) {
        ACameraMetadata_free(camera_metadata);
    }
    if (camera_id_list) {
        ACameraManager_deleteCameraIdList(camera_id_list);
    }
    ACameraManager_delete(camera_manager);

    return status == ACAMERA_OK;
}

bool setup_image_reader(DroidMediaCamera *camera)
{
    media_status_t media_status;

    if (camera->m_image_reader) {
        AImageReader_delete(camera->m_image_reader);
        camera->m_image_reader = NULL;
    }

    media_status = AImageReader_new(
        camera->image_width, camera->image_height, camera->image_format,
        2, &camera->m_image_reader);
    if (media_status != AMEDIA_OK) {
        ALOGE("Create image reader. status %d", media_status);
        goto fail;
    }

    if (camera->m_image_reader == NULL) {
        ALOGE("null image reader created");
        goto fail;
    }

    media_status = AImageReader_setImageListener(camera->m_image_reader, &camera->m_image_listener);
    if (media_status != AMEDIA_OK) {
        ALOGE("Set AImageReader listener failed. status %d", media_status);
        goto fail;
    }

    media_status = AImageReader_getWindow(camera->m_image_reader, &camera->m_image_reader_anw);
    if (media_status != AMEDIA_OK) {
        ALOGE("AImageReader_getWindow failed. status %d", media_status);
        goto fail;
    }

//    ANativeWindow_acquire(camera->m_image_reader_anw);
    if (camera->m_image_reader_anw == NULL) {
        ALOGE("null ANativeWindow from AImageReader");
        goto fail;
    }

    return true;

fail:
    if (camera->m_image_reader) {
        AImageReader_delete(camera->m_image_reader);
        camera->m_image_reader = NULL;
    }

    return false;
}

void destroy_capture_session(DroidMediaCamera *camera)
{
    if (camera->m_image_reader_output_target) {
        ACameraOutputTarget_free(camera->m_image_reader_output_target);
        camera->m_image_reader_output_target = NULL;
    }

    if (camera->m_image_reader_output) {
        ACaptureSessionOutput_free(camera->m_image_reader_output);
        camera->m_image_reader_output = NULL;
    }

    if (camera->m_preview_output_target) {
        ACameraOutputTarget_free(camera->m_preview_output_target);
        camera->m_preview_output_target = NULL;
    }

    if (camera->m_preview_output) {
        ACaptureSessionOutput_free(camera->m_preview_output);
        camera->m_preview_output = NULL;
    }

    if (camera->m_video_output_target) {
        ACameraOutputTarget_free(camera->m_video_output_target);
        camera->m_video_output_target = NULL;
    }

    if (camera->m_video_output) {
        ACaptureSessionOutput_free(camera->m_video_output);
        camera->m_video_output = NULL;
    }

    if (camera->m_capture_session_output_container) {
        ACaptureSessionOutputContainer_free(camera->m_capture_session_output_container);
        camera->m_capture_session_output_container = NULL;
    }

    if (camera->m_preview_request) {
        ACaptureRequest_free(camera->m_preview_request);
        camera->m_preview_request = NULL;
    }

    if (camera->m_image_request != NULL) {
        ACaptureRequest_free(camera->m_image_request);
        camera->m_image_request = NULL;
    }

    if (camera->m_video_request) {
        ACaptureRequest_free(camera->m_video_request);
        camera->m_video_request = NULL;
    }

    if (camera->m_session) {
        ACameraCaptureSession_close(camera->m_session);
        camera->m_session = NULL;
    }
}

bool setup_capture_session(DroidMediaCamera *camera)
{
    camera_status_t status;

    ALOGI("setup_capture_session start");

    camera->m_queue->setBufferSize(camera->preview_width, camera->preview_height);

    status = ACaptureSessionOutputContainer_create(&camera->m_capture_session_output_container);
    if (status != ACAMERA_OK) {
        goto fail;
    }

    // Preview
    ALOGI("setup_capture_session preview");
    status = ACameraDevice_createCaptureRequest(camera->m_device,
        TEMPLATE_PREVIEW, &camera->m_preview_request);
    if (status != ACAMERA_OK) {
        goto fail;
    }

    status = ACameraOutputTarget_create(camera->m_queue->window(), &camera->m_preview_output_target);
    if (status != ACAMERA_OK) {
        goto fail;
    }

    status = ACaptureRequest_addTarget(camera->m_preview_request, camera->m_preview_output_target);
    if (status != ACAMERA_OK) {
        goto fail;
    }

    status = ACaptureSessionOutput_create(camera->m_queue->window(), &camera->m_preview_output);
    if (status != ACAMERA_OK) {
        goto fail;
    }

    status = ACaptureSessionOutputContainer_add(camera->m_capture_session_output_container,
        camera->m_preview_output);
    if (status != ACAMERA_OK) {
        goto fail;
    }
    ALOGI("setup_capture_session preview done");

    if (camera->m_video_mode) {
        // Video mode
        ALOGI("setup_capture_session video start");
        camera->m_queue->setBufferSize(camera->video_width, camera->video_height);

        status = ACameraOutputTarget_create(camera->m_recording_queue->window(), &camera->m_video_output_target);
        if (status != ACAMERA_OK) {
            goto fail;
        }

        status = ACaptureRequest_addTarget(camera->m_video_request, camera->m_video_output_target);
        if (status != ACAMERA_OK) {
            goto fail;
        }

        status = ACaptureSessionOutput_create(camera->m_recording_queue->window(), &camera->m_video_output);
        if (status != ACAMERA_OK) {
            goto fail;
        }

        status = ACaptureSessionOutputContainer_add(camera->m_capture_session_output_container,
            camera->m_video_output);
        if (status != ACAMERA_OK) {
            goto fail;
        }
        ALOGI("setup_capture_session video done");
    } else {
        // Image mode
        ALOGI("setup_capture_session image start");
        status = ACameraDevice_createCaptureRequest(camera->m_device,
            TEMPLATE_STILL_CAPTURE, &camera->m_image_request);
        if (status != ACAMERA_OK) {
            goto fail;
        }

        status = ACameraOutputTarget_create(camera->m_image_reader_anw, &camera->m_image_reader_output_target);
        if (status != ACAMERA_OK) {
            goto fail;
        }

        status = ACaptureRequest_addTarget(camera->m_image_request, camera->m_image_reader_output_target);
        if (status != ACAMERA_OK) {
            goto fail;
        }

        status = ACaptureSessionOutput_create(camera->m_image_reader_anw, &camera->m_image_reader_output);
        if (status != ACAMERA_OK) {
            goto fail;
        }

        status = ACaptureSessionOutputContainer_add(camera->m_capture_session_output_container,
            camera->m_image_reader_output);
        if (status != ACAMERA_OK) {
            goto fail;
        }
        ALOGI("setup_capture_session image done");
    }

    status = ACameraDevice_createCaptureSession(
        camera->m_device, camera->m_capture_session_output_container,
        &camera->m_capture_session_state_callbacks, &camera->m_session);
    if (status != ACAMERA_OK) {
        goto fail;
    }

    ALOGI("setup_capture_session done");
    return true;

fail:
    destroy_capture_session(camera);

    return false;
}

DroidMediaCamera *droid_media_camera_connect(int camera_number)
{
    camera_status_t status;
    ACameraDevice *camera_device = NULL;
    DroidMediaCamera *camera = new DroidMediaCamera;
    android::sp<DroidMediaBufferQueue> queue;
    android::sp<DroidMediaBufferQueue> recording_queue;
    const char *selected_camera_id = NULL;

    if (!camera) {
        ALOGE("Failed to allocate DroidMediaCamera");
        return NULL;
    }

    camera->m_manager = ACameraManager_create();

    ALOGI("Opening camera %i", camera_number);

    status = ACameraManager_getCameraIdList(camera->m_manager, &camera->m_camera_id_list);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to get camera id list: %d", status);
        goto fail;
    }

    if (camera->m_camera_id_list->numCameras <= 0 || camera_number >= camera->m_camera_id_list->numCameras) {
        ALOGE("Invalid camera number");
        goto fail;
    }

    selected_camera_id = camera->m_camera_id_list->cameraIds[camera_number];
    ALOGI("Selected camera %s", selected_camera_id);

    // Set device state callbacks
    camera->m_device_state_callbacks.context = camera;
    camera->m_device_state_callbacks.onDisconnected = device_on_disconnected;
    camera->m_device_state_callbacks.onError = device_on_error;

    status = ACameraManager_openCamera(camera->m_manager, selected_camera_id, &camera->m_device_state_callbacks, &camera_device);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to open camera %s with error: %i", selected_camera_id, status);
        goto fail;
    }

    ALOGI("Camera %s opened", selected_camera_id);
    camera->m_device = camera_device;

    status = ACameraManager_getCameraCharacteristics(camera->m_manager,
                                                     selected_camera_id,
                                                     &camera->m_metadata);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to get camera characteristics: %d", status);
        goto fail;
    }

    queue = new DroidMediaBufferQueue("DroidMediaCameraBufferQueue");
    if (!queue->connectListener()) {
        ALOGE("Failed to connect buffer queue listener");
        goto fail;
    }

    camera->m_queue = queue;

#if ANDROID_MAJOR >= 9
    recording_queue = new DroidMediaBufferQueue("DroidMediaCameraBufferRecordingQueue");
    if (!recording_queue->connectListener()) {
        ALOGE("Failed to connect video buffer queue listener");
    } else {
      camera->m_recording_queue = recording_queue;
    }
#endif

    // Set capture session callbacks
    camera->m_capture_session_state_callbacks.context = camera;
    camera->m_capture_session_state_callbacks.onReady = capture_session_on_ready;
    camera->m_capture_session_state_callbacks.onActive = capture_session_on_active;
    camera->m_capture_session_state_callbacks.onClosed = capture_session_on_closed;

    // Set capture callbacks
    camera->m_capture_callbacks.context = camera;
    camera->m_capture_callbacks.onCaptureStarted = capture_session_on_capture_started;
    camera->m_capture_callbacks.onCaptureProgressed = capture_session_on_capture_progressed;
    camera->m_capture_callbacks.onCaptureCompleted = capture_session_on_capture_completed;
    camera->m_capture_callbacks.onCaptureFailed = capture_session_on_capture_failed;
    camera->m_capture_callbacks.onCaptureSequenceCompleted = capture_session_on_capture_sequence_completed;
    camera->m_capture_callbacks.onCaptureSequenceAborted = capture_session_on_capture_sequence_abort;
    camera->m_capture_callbacks.onCaptureBufferLost = capture_session_on_capture_buffer_lost;

    // TODO setup still image reader etc
    camera->m_image_listener.context = camera;
    camera->m_image_listener.onImageAvailable = &still_image_available;

    return camera;

fail:
    if (camera->m_metadata) {
        ACameraMetadata_free(camera->m_metadata);
        camera->m_metadata = NULL;
    }

    if (camera->m_device != NULL) {
        status = ACameraDevice_close(camera->m_device);

        if (status != ACAMERA_OK) {
            ALOGE("Failed to close CameraDevice.");
        }
        camera->m_device = NULL;
    }

    if (camera->m_camera_id_list) {
        ACameraManager_deleteCameraIdList(camera->m_camera_id_list);
    }

    if (camera->m_manager) {
        ACameraManager_delete(camera->m_manager);
    }

    return NULL;
}

bool droid_media_camera_reconnect(DroidMediaCamera *camera)
{
    return false;
}

void droid_media_camera_disconnect(DroidMediaCamera *camera)
{
    ALOGI("disconnect");
    destroy_capture_session(camera);

    if (camera->m_image_reader) {
        AImageReader_delete(camera->m_image_reader);
        camera->m_image_reader = NULL;
    }

    if (camera->m_device != NULL) {
        ACameraDevice_close(camera->m_device);
        camera->m_device = NULL;
    }

    if (camera->m_metadata) {
        ACameraMetadata_free(camera->m_metadata);
        camera->m_metadata = NULL;
    }

    if (camera->m_camera_id_list) {
        ACameraManager_deleteCameraIdList(camera->m_camera_id_list);
        camera->m_camera_id_list = NULL;
    }

    camera->m_queue->setCallbacks(0, 0);

#if ANDROID_MAJOR >= 9
    if (camera->m_recording_queue.get()) {
        camera->m_recording_queue->setCallbacks(0, 0);
    }
#endif

    if (camera->m_manager) {
        ACameraManager_delete(camera->m_manager);
        camera->m_manager = NULL;
    }

    delete camera;
}

bool droid_media_camera_lock(DroidMediaCamera *camera)
{
    // TODO Is camera lock needed?
    return true;
}

bool droid_media_camera_unlock(DroidMediaCamera *camera)
{
    // TODO Is camera unlock needed?
    return true;
}

bool droid_media_camera_start_preview(DroidMediaCamera *camera)
{
    ALOGI("start_preview");
    camera_status_t status;
    if (camera->m_preview_enabled) {
        return true;
    }

    if (!setup_capture_session(camera)) {
        ALOGE("Failed to setup capture session");
        return false;
    }

    status = ACameraCaptureSession_setRepeatingRequest(camera->m_session, &camera->m_capture_callbacks, 1,
        &camera->m_preview_request, NULL);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to start preview");
        return false;
    }

    camera->m_preview_enabled = true;
    ALOGI("start_preview success");

    return true;
}

void droid_media_camera_stop_preview(DroidMediaCamera *camera)
{
    ALOGI("stop_preview");
    if (camera->m_session) {
        ALOGE("Stopping preview");
        camera->m_preview_enabled = false;
        ACameraCaptureSession_stopRepeating(camera->m_session);
    }

    destroy_capture_session(camera);
}

bool droid_media_camera_is_preview_enabled(DroidMediaCamera *camera)
{
    ALOGI("is_preview_enabled");
    return camera->m_preview_enabled;
}

bool droid_media_camera_start_recording(DroidMediaCamera *camera)
{
    // TODO start recording
    ALOGI("start_recording");

    return false;
}

void droid_media_camera_stop_recording(DroidMediaCamera *camera)
{
    // TODO stop recording
    ALOGI("stop_recording");
}

bool droid_media_camera_is_recording_enabled(DroidMediaCamera *camera)
{
    ALOGI("is_recording_enabled");
    return camera->m_video_recording_enabled;
}

bool droid_media_camera_start_auto_focus(DroidMediaCamera *camera)
{
    ALOGI("start_auto_focus");
    camera_status_t status;
    uint8_t afTrigger = ACAMERA_CONTROL_AF_TRIGGER_START;

    ACameraCaptureSession_stopRepeating(camera->m_session);

    status = ACaptureRequest_setEntry_u8(camera->m_preview_request,
        ACAMERA_CONTROL_AF_TRIGGER, 1, &afTrigger);

    if (status == ACAMERA_OK) {
        status = ACameraCaptureSession_capture(camera->m_session,
            &camera->m_capture_callbacks, 1, &camera->m_preview_request, NULL);
    }

    return status == ACAMERA_OK;
}

bool droid_media_camera_cancel_auto_focus(DroidMediaCamera *camera)
{
    ALOGI("cancel_auto_focus");
    camera_status_t status;
    uint8_t afTrigger = ACAMERA_CONTROL_AF_TRIGGER_CANCEL;

    ACameraCaptureSession_stopRepeating(camera->m_session);

    status = ACaptureRequest_setEntry_u8(camera->m_preview_request,
        ACAMERA_CONTROL_AF_TRIGGER, 1, &afTrigger);

    if (status == ACAMERA_OK) {
        status = ACameraCaptureSession_capture(camera->m_session,
            &camera->m_capture_callbacks, 1, &camera->m_preview_request, NULL);
    }

    return status == ACAMERA_OK;
}

void droid_media_camera_set_callbacks(DroidMediaCamera *camera, DroidMediaCameraCallbacks *cb, void *data)
{
    ALOGI("set_callbacks");
    memcpy(&camera->m_cb, cb, sizeof(camera->m_cb));
    camera->m_cb_data = data;
}

bool droid_media_camera_send_command(DroidMediaCamera *camera, int32_t cmd, int32_t arg1, int32_t arg2)
{
    ALOGI("send_command");
    // TODO Is send command needed?
    return false;
}

bool droid_media_camera_store_meta_data_in_buffers(DroidMediaCamera *camera, bool enabled)
{
    // TODO store metadata in buffers
    return false;
}

void droid_media_camera_set_preview_callback_flags(DroidMediaCamera *camera, int preview_callback_flag)
{
    // TODO set preview callback flags
    ALOGI("set_preview_callback_flags");
//    camera->m_camera->setPreviewCallbackFlags(preview_callback_flag);
}

int wb_mode_string_to_enum(const char *wb_mode)
{
    return
        !wb_mode ?
            ACAMERA_CONTROL_AWB_MODE_AUTO :
        !strcmp(wb_mode, android::CameraParameters::WHITE_BALANCE_AUTO) ?
            ACAMERA_CONTROL_AWB_MODE_AUTO :
        !strcmp(wb_mode, android::CameraParameters::WHITE_BALANCE_INCANDESCENT) ?
            ACAMERA_CONTROL_AWB_MODE_INCANDESCENT :
        !strcmp(wb_mode, android::CameraParameters::WHITE_BALANCE_FLUORESCENT) ?
            ACAMERA_CONTROL_AWB_MODE_FLUORESCENT :
        !strcmp(wb_mode, android::CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT) ?
            ACAMERA_CONTROL_AWB_MODE_WARM_FLUORESCENT :
        !strcmp(wb_mode, android::CameraParameters::WHITE_BALANCE_DAYLIGHT) ?
            ACAMERA_CONTROL_AWB_MODE_DAYLIGHT :
        !strcmp(wb_mode, android::CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT) ?
            ACAMERA_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT :
        !strcmp(wb_mode, android::CameraParameters::WHITE_BALANCE_TWILIGHT) ?
            ACAMERA_CONTROL_AWB_MODE_TWILIGHT :
        !strcmp(wb_mode, android::CameraParameters::WHITE_BALANCE_SHADE) ?
            ACAMERA_CONTROL_AWB_MODE_SHADE :
        -1;
}

const char *wb_mode_enum_to_string(uint8_t wb_mode, bool &found)
{
    found = true;
    switch (wb_mode) {
        case ACAMERA_CONTROL_AWB_MODE_OFF:
            found = false;
            return "";
        case ACAMERA_CONTROL_AWB_MODE_AUTO:
            return android::CameraParameters::WHITE_BALANCE_AUTO;
        case ACAMERA_CONTROL_AWB_MODE_INCANDESCENT:
            return android::CameraParameters::WHITE_BALANCE_INCANDESCENT;
        case ACAMERA_CONTROL_AWB_MODE_FLUORESCENT:
            return android::CameraParameters::WHITE_BALANCE_FLUORESCENT;
        case ACAMERA_CONTROL_AWB_MODE_WARM_FLUORESCENT:
            return android::CameraParameters::WHITE_BALANCE_WARM_FLUORESCENT;
        case ACAMERA_CONTROL_AWB_MODE_DAYLIGHT:
            return android::CameraParameters::WHITE_BALANCE_DAYLIGHT;
        case ACAMERA_CONTROL_AWB_MODE_CLOUDY_DAYLIGHT:
            return android::CameraParameters::WHITE_BALANCE_CLOUDY_DAYLIGHT;
        case ACAMERA_CONTROL_AWB_MODE_TWILIGHT:
            return android::CameraParameters::WHITE_BALANCE_TWILIGHT;
        case ACAMERA_CONTROL_AWB_MODE_SHADE:
            return android::CameraParameters::WHITE_BALANCE_SHADE;
        default:
            found = false;
            ALOGE("%s: Unknown AWB mode enum: %d",
                    __FUNCTION__, wb_mode);
            return "";
    }
}

int effect_mode_string_to_enum(const char *effect_mode)
{
    return
        !effect_mode ?
            ACAMERA_CONTROL_EFFECT_MODE_OFF :
        !strcmp(effect_mode, android::CameraParameters::EFFECT_NONE) ?
            ACAMERA_CONTROL_EFFECT_MODE_OFF :
        !strcmp(effect_mode, android::CameraParameters::EFFECT_MONO) ?
            ACAMERA_CONTROL_EFFECT_MODE_MONO :
        !strcmp(effect_mode, android::CameraParameters::EFFECT_NEGATIVE) ?
            ACAMERA_CONTROL_EFFECT_MODE_NEGATIVE :
        !strcmp(effect_mode, android::CameraParameters::EFFECT_SOLARIZE) ?
            ACAMERA_CONTROL_EFFECT_MODE_SOLARIZE :
        !strcmp(effect_mode, android::CameraParameters::EFFECT_SEPIA) ?
            ACAMERA_CONTROL_EFFECT_MODE_SEPIA :
        !strcmp(effect_mode, android::CameraParameters::EFFECT_POSTERIZE) ?
            ACAMERA_CONTROL_EFFECT_MODE_POSTERIZE :
        !strcmp(effect_mode, android::CameraParameters::EFFECT_WHITEBOARD) ?
            ACAMERA_CONTROL_EFFECT_MODE_WHITEBOARD :
        !strcmp(effect_mode, android::CameraParameters::EFFECT_BLACKBOARD) ?
            ACAMERA_CONTROL_EFFECT_MODE_BLACKBOARD :
        !strcmp(effect_mode, android::CameraParameters::EFFECT_AQUA) ?
            ACAMERA_CONTROL_EFFECT_MODE_AQUA :
        -1;
}

const char *effect_mode_enum_to_string(uint8_t effect_mode, bool &found)
{
    found = true;
    switch (effect_mode) {
        case ACAMERA_CONTROL_EFFECT_MODE_OFF:
            return android::CameraParameters::EFFECT_NONE;
        case ACAMERA_CONTROL_EFFECT_MODE_MONO:
            return android::CameraParameters::EFFECT_MONO;
        case ACAMERA_CONTROL_EFFECT_MODE_NEGATIVE:
            return android::CameraParameters::EFFECT_NEGATIVE;
        case ACAMERA_CONTROL_EFFECT_MODE_SOLARIZE:
            return android::CameraParameters::EFFECT_SOLARIZE;
        case ACAMERA_CONTROL_EFFECT_MODE_SEPIA:
            return android::CameraParameters::EFFECT_SEPIA;
        case ACAMERA_CONTROL_EFFECT_MODE_POSTERIZE:
            return android::CameraParameters::EFFECT_POSTERIZE;
        case ACAMERA_CONTROL_EFFECT_MODE_WHITEBOARD:
            return android::CameraParameters::EFFECT_WHITEBOARD;
        case ACAMERA_CONTROL_EFFECT_MODE_BLACKBOARD:
            return android::CameraParameters::EFFECT_BLACKBOARD;
        case ACAMERA_CONTROL_EFFECT_MODE_AQUA:
            return android::CameraParameters::EFFECT_AQUA;
        default:
            found = false;
            ALOGE("%s: Unknown effect mode enum: %d",
                    __FUNCTION__, effect_mode);
            return "";
    }
}

int ab_mode_string_to_enum(const char *ab_mode)
{
    return
        !ab_mode ?
            ACAMERA_CONTROL_AE_ANTIBANDING_MODE_AUTO :
        !strcmp(ab_mode, android::CameraParameters::ANTIBANDING_AUTO) ?
            ACAMERA_CONTROL_AE_ANTIBANDING_MODE_AUTO :
        !strcmp(ab_mode, android::CameraParameters::ANTIBANDING_OFF) ?
            ACAMERA_CONTROL_AE_ANTIBANDING_MODE_OFF :
        !strcmp(ab_mode, android::CameraParameters::ANTIBANDING_50HZ) ?
            ACAMERA_CONTROL_AE_ANTIBANDING_MODE_50HZ :
        !strcmp(ab_mode, android::CameraParameters::ANTIBANDING_60HZ) ?
            ACAMERA_CONTROL_AE_ANTIBANDING_MODE_60HZ :
        -1;
}

const char *ab_mode_enum_to_string(uint8_t ab_mode, bool &found)
{
    found = true;
    switch (ab_mode) {
        case ACAMERA_CONTROL_AE_ANTIBANDING_MODE_AUTO:
            return android::CameraParameters::ANTIBANDING_AUTO;
        case ACAMERA_CONTROL_AE_ANTIBANDING_MODE_OFF:
            return android::CameraParameters::ANTIBANDING_OFF;
        case ACAMERA_CONTROL_AE_ANTIBANDING_MODE_50HZ:
            return android::CameraParameters::ANTIBANDING_50HZ;
        case ACAMERA_CONTROL_AE_ANTIBANDING_MODE_60HZ:
            return android::CameraParameters::ANTIBANDING_60HZ;
        default:
            found = false;
            ALOGE("%s: Unknown antibanding mode enum: %d",
                    __FUNCTION__, ab_mode);
            return "";
    }
}

int scene_mode_string_to_enum(const char *scene_mode, uint8_t default_scene_mode)
{
    return
        !scene_mode ?
            default_scene_mode :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_AUTO) ?
            default_scene_mode :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_ACTION) ?
            ACAMERA_CONTROL_SCENE_MODE_ACTION :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_PORTRAIT) ?
            ACAMERA_CONTROL_SCENE_MODE_PORTRAIT :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_LANDSCAPE) ?
            ACAMERA_CONTROL_SCENE_MODE_LANDSCAPE :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_NIGHT) ?
            ACAMERA_CONTROL_SCENE_MODE_NIGHT :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_NIGHT_PORTRAIT) ?
            ACAMERA_CONTROL_SCENE_MODE_NIGHT_PORTRAIT :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_THEATRE) ?
            ACAMERA_CONTROL_SCENE_MODE_THEATRE :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_BEACH) ?
            ACAMERA_CONTROL_SCENE_MODE_BEACH :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_SNOW) ?
            ACAMERA_CONTROL_SCENE_MODE_SNOW :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_SUNSET) ?
            ACAMERA_CONTROL_SCENE_MODE_SUNSET :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_STEADYPHOTO) ?
            ACAMERA_CONTROL_SCENE_MODE_STEADYPHOTO :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_FIREWORKS) ?
            ACAMERA_CONTROL_SCENE_MODE_FIREWORKS :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_SPORTS) ?
            ACAMERA_CONTROL_SCENE_MODE_SPORTS :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_PARTY) ?
            ACAMERA_CONTROL_SCENE_MODE_PARTY :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_CANDLELIGHT) ?
            ACAMERA_CONTROL_SCENE_MODE_CANDLELIGHT :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_BARCODE) ?
            ACAMERA_CONTROL_SCENE_MODE_BARCODE :
        !strcmp(scene_mode, android::CameraParameters::SCENE_MODE_HDR) ?
            ACAMERA_CONTROL_SCENE_MODE_HDR :
        -1;
}

const char *scene_mode_enum_to_string(uint8_t scene_mode, bool &found)
{
    found = true;
    switch (scene_mode) {
        case ACAMERA_CONTROL_SCENE_MODE_DISABLED:
        case ACAMERA_CONTROL_SCENE_MODE_FACE_PRIORITY:
            return android::CameraParameters::SCENE_MODE_AUTO;
        case ACAMERA_CONTROL_SCENE_MODE_ACTION:
            return android::CameraParameters::SCENE_MODE_ACTION;
        case ACAMERA_CONTROL_SCENE_MODE_PORTRAIT:
            return android::CameraParameters::SCENE_MODE_PORTRAIT;
        case ACAMERA_CONTROL_SCENE_MODE_LANDSCAPE:
            return android::CameraParameters::SCENE_MODE_LANDSCAPE;
        case ACAMERA_CONTROL_SCENE_MODE_NIGHT:
            return android::CameraParameters::SCENE_MODE_NIGHT;
        case ACAMERA_CONTROL_SCENE_MODE_NIGHT_PORTRAIT:
            return android::CameraParameters::SCENE_MODE_NIGHT_PORTRAIT;
        case ACAMERA_CONTROL_SCENE_MODE_THEATRE:
            return android::CameraParameters::SCENE_MODE_THEATRE;
        case ACAMERA_CONTROL_SCENE_MODE_BEACH:
            return android::CameraParameters::SCENE_MODE_BEACH;
        case ACAMERA_CONTROL_SCENE_MODE_SNOW:
            return android::CameraParameters::SCENE_MODE_SNOW;
        case ACAMERA_CONTROL_SCENE_MODE_SUNSET:
            return android::CameraParameters::SCENE_MODE_SUNSET;
        case ACAMERA_CONTROL_SCENE_MODE_STEADYPHOTO:
            return android::CameraParameters::SCENE_MODE_STEADYPHOTO;
        case ACAMERA_CONTROL_SCENE_MODE_FIREWORKS:
            return android::CameraParameters::SCENE_MODE_FIREWORKS;
        case ACAMERA_CONTROL_SCENE_MODE_SPORTS:
            return android::CameraParameters::SCENE_MODE_SPORTS;
        case ACAMERA_CONTROL_SCENE_MODE_PARTY:
            return android::CameraParameters::SCENE_MODE_PARTY;
        case ACAMERA_CONTROL_SCENE_MODE_CANDLELIGHT:
            return android::CameraParameters::SCENE_MODE_CANDLELIGHT;
        case ACAMERA_CONTROL_SCENE_MODE_BARCODE:
            return android::CameraParameters::SCENE_MODE_BARCODE;
        case ACAMERA_CONTROL_SCENE_MODE_HDR:
            return android::CameraParameters::SCENE_MODE_HDR;
        default:
            found = false;
            ALOGE("%s: Unknown scene mode enum: %d",
                    __FUNCTION__, scene_mode);
            return "";
    }
}

int flash_mode_string_to_enum(const char *flash_mode)
{
    return
        !flash_mode ?
            ACAMERA_CONTROL_AE_MODE_ON :
        !strcmp(flash_mode, android::CameraParameters::FLASH_MODE_OFF) ?
            ACAMERA_CONTROL_AE_MODE_ON :
        !strcmp(flash_mode, android::CameraParameters::FLASH_MODE_AUTO) ?
            ACAMERA_CONTROL_AE_MODE_ON_AUTO_FLASH :
        !strcmp(flash_mode, android::CameraParameters::FLASH_MODE_ON) ?
            ACAMERA_CONTROL_AE_MODE_ON_ALWAYS_FLASH :
        !strcmp(flash_mode, android::CameraParameters::FLASH_MODE_RED_EYE) ?
            ACAMERA_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE :
/*
        !strcmp(flash_mode, android::CameraParameters::FLASH_MODE_TORCH) ?
             :
*/
        -1;
}
/*
const char *flash_mode_enum_to_string(uint8_t flash_mode, bool &found)
{
    found = true;
    switch (flash_mode) {
        case ACAMERA_CONTROL_AE_MODE_ON:
            return android::CameraParameters::FLASH_MODE_OFF;
        case ACAMERA_CONTROL_AE_MODE_ON_AUTO_FLASH:
            return android::CameraParameters::FLASH_MODE_AUTO;
        case ACAMERA_CONTROL_AE_MODE_ON_ALWAYS_FLASH:
            return android::CameraParameters::FLASH_MODE_ON;
        case ACAMERA_CONTROL_AE_MODE_ON_AUTO_FLASH_REDEYE:
            return android::CameraParameters::FLASH_MODE_RED_EYE;
//        case FLASH_MODE_TORCH:
//            return android::CameraParameters::FLASH_MODE_TORCH;
        default:
            ALOGE("%s: Unknown flash mode enum %d",
                    __FUNCTION__, flash_mode);
            found = false;
            return "";
    }
}
*/

int focus_mode_string_to_enum(const char *focus_mode)
{
    return
        !focus_mode ?
            ACAMERA_CONTROL_AF_MODE_OFF :
        !strcmp(focus_mode, android::CameraParameters::FOCUS_MODE_AUTO) ?
            ACAMERA_CONTROL_AF_MODE_AUTO :
        !strcmp(focus_mode, android::CameraParameters::FOCUS_MODE_INFINITY) ?
            ACAMERA_CONTROL_AF_MODE_OFF :
        !strcmp(focus_mode, android::CameraParameters::FOCUS_MODE_MACRO) ?
            ACAMERA_CONTROL_AF_MODE_MACRO :
        !strcmp(focus_mode, android::CameraParameters::FOCUS_MODE_FIXED) ?
            ACAMERA_CONTROL_AF_MODE_OFF :
        !strcmp(focus_mode, android::CameraParameters::FOCUS_MODE_EDOF) ?
            ACAMERA_CONTROL_AF_MODE_EDOF :
        !strcmp(focus_mode, android::CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO) ?
            ACAMERA_CONTROL_AF_MODE_CONTINUOUS_VIDEO :
        !strcmp(focus_mode, android::CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE) ?
            ACAMERA_CONTROL_AF_MODE_CONTINUOUS_PICTURE :
        -1;
}

const char *focus_mode_enum_to_string(uint8_t focus_mode, bool fixed_lens, bool &found)
{
    found = true;
    switch (focus_mode) {
        case ACAMERA_CONTROL_AF_MODE_AUTO:
            return android::CameraParameters::FOCUS_MODE_AUTO;
        case ACAMERA_CONTROL_AF_MODE_MACRO:
            return android::CameraParameters::FOCUS_MODE_MACRO;
        case ACAMERA_CONTROL_AF_MODE_CONTINUOUS_VIDEO:
            return android::CameraParameters::FOCUS_MODE_CONTINUOUS_VIDEO;
        case ACAMERA_CONTROL_AF_MODE_CONTINUOUS_PICTURE:
            return android::CameraParameters::FOCUS_MODE_CONTINUOUS_PICTURE;
        case ACAMERA_CONTROL_AF_MODE_EDOF:
            return android::CameraParameters::FOCUS_MODE_EDOF;
        case ANDROID_CONTROL_AF_MODE_OFF:
            if (fixed_lens) {
                return android::CameraParameters::FOCUS_MODE_FIXED;
            } else {
                return android::CameraParameters::FOCUS_MODE_INFINITY;
            }
        default:
            ALOGE("%s: Unknown focus mode enum: %d",
                    __FUNCTION__, focus_mode);
            found = false;
            return "";
    }
}

int param_key_string_to_enum(const char *key)
{
    return
        !strcmp(key, android::CameraParameters::KEY_PREVIEW_FPS_RANGE) ?
            ACAMERA_CONTROL_AE_TARGET_FPS_RANGE :
        !strcmp(key, android::CameraParameters::KEY_JPEG_QUALITY) ?
            ACAMERA_JPEG_QUALITY :
        !strcmp(key, android::CameraParameters::KEY_WHITE_BALANCE) ?
            ACAMERA_CONTROL_AWB_MODE :
        !strcmp(key, android::CameraParameters::KEY_EFFECT) ?
            ACAMERA_CONTROL_EFFECT_MODE :
        !strcmp(key, android::CameraParameters::KEY_ANTIBANDING) ?
            ACAMERA_CONTROL_AE_ANTIBANDING_MODE :
        !strcmp(key, android::CameraParameters::KEY_SCENE_MODE) ?
            ACAMERA_CONTROL_SCENE_MODE :
        !strcmp(key, android::CameraParameters::KEY_FLASH_MODE) ?
            ACAMERA_FLASH_MODE :
        !strcmp(key, android::CameraParameters::KEY_FOCUS_MODE) ?
            ACAMERA_CONTROL_AF_MODE :
        !strcmp(key, android::CameraParameters::KEY_FOCUS_AREAS) ?
            ACAMERA_CONTROL_AF_REGIONS :
        !strcmp(key, android::CameraParameters::KEY_EXPOSURE_COMPENSATION) ?
            ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION :
        !strcmp(key, android::CameraParameters::KEY_AUTO_EXPOSURE_LOCK) ?
            ACAMERA_CONTROL_AE_LOCK :
        !strcmp(key, android::CameraParameters::KEY_AUTO_WHITEBALANCE_LOCK) ?
            ACAMERA_CONTROL_AWB_LOCK :
        !strcmp(key, android::CameraParameters::KEY_METERING_AREAS) ?
            ACAMERA_CONTROL_AE_REGIONS :
        !strcmp(key, android::CameraParameters::KEY_ZOOM) ?
            ACAMERA_CONTROL_ZOOM_RATIO :
        !strcmp(key, android::CameraParameters::KEY_VIDEO_STABILIZATION) ?
            ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE :
        -1;
}

bool parse_areas(std::string &str, std::vector<int32_t> *areas)
{
    static const size_t NUM_FIELDS = 5;
    areas->clear();
    if (str.empty()) {
        // If no key exists, use default (0,0,0,0,0)
        areas->insert(areas->end(), {0, 0, 0, 0, 0});
        return true;
    }
    ssize_t start = str.find('(', 0) + 1;
    while (start != 0) {
        const char *area = str.c_str() + start;
        char *num_end;
        int values[NUM_FIELDS];
        for (size_t i = 0; i < NUM_FIELDS; i++) {
            errno = 0;
            values[i] = strtol(area, &num_end, 10);
            if (errno || num_end == area) {
                return false;
            }
            area = num_end + 1;
        }
        areas->insert(areas->end(), {values[0], values[1], values[2], values[3], values[4]});
        start = str.find('(', start) + 1;
    }
    return true;
}

void parse_pair_string(std::string &str, char delim, std::string &first, std::string &second)
{
    std::istringstream iss(str);
    std::getline(iss, first, delim);
    std::getline(iss, second, delim);
}

void parse_pair_int32(std::string &str, char delim, int32_t &first, int32_t &second)
{
    std::string first_s;
    std::string second_s;
    parse_pair_string(str, delim, first_s, second_s);
    first = std::stoi(first_s);
    second = std::stoi(second_s);
}

void update_request(DroidMediaCamera *camera, ACaptureRequest *request, std::unordered_map<std::string, std::string> &param_map) {
     ALOGI("update_request");
     uint8_t controlMode = ACAMERA_CONTROL_MODE_AUTO;
     ACaptureRequest_setEntry_u8(request,
         ACAMERA_CONTROL_MODE, 1, &controlMode);

     // TODO check if something is missing
     for (auto& it: param_map) {
         std::string key_s = it.first;
         std::string value_s = it.second;
         ALOGI("set_parameters %s=%s", key_s.c_str(), value_s.c_str());
         int32_t key;
         if ((key = param_key_string_to_enum(key_s.c_str())) >= 0) {
             switch (key) {
             case ACAMERA_CONTROL_AE_ANTIBANDING_MODE: {
                 uint8_t mode;
                 if ((mode = ab_mode_string_to_enum(value_s.c_str())) != -1) {
                     ACaptureRequest_setEntry_u8(request, key, 1, &mode);
                 }
                 break;
             }
             case ACAMERA_CONTROL_AE_EXPOSURE_COMPENSATION:
                 if (int32_t value = std::stoi(value_s)) {
                     ACaptureRequest_setEntry_i32(request, key, 1, &value);
                 }
                 break;
             case ACAMERA_CONTROL_AE_LOCK: {
                 uint8_t value = ACAMERA_CONTROL_AE_LOCK_OFF;
                 if (!strcmp(value_s.c_str(), android::CameraParameters::TRUE)) {
                     value = ACAMERA_CONTROL_AE_LOCK_ON;
                 }
                 ACaptureRequest_setEntry_u8(request, key, 1, &value);
                 break;
             }
             case ACAMERA_CONTROL_AE_REGIONS: {
                 std::vector<int32_t> areas;
                 if (parse_areas(value_s, &areas)) {
                     int32_t *values = new int32_t[areas.size()];
                     for (int i = 0; i < areas.size(); i++) {
                         values[i] = areas[i];
                     }
                     ACaptureRequest_setEntry_i32(request, key, areas.size(), values);
                     ACaptureRequest_setEntry_i32(request, ACAMERA_CONTROL_AWB_REGIONS, areas.size(), values);
                     delete[] values;
                 }
                 break;
             }
             case ACAMERA_CONTROL_AE_TARGET_FPS_RANGE: {
                 int32_t values[2];
                 parse_pair_int32(value_s, ',', values[0], values[1]);
                 ACaptureRequest_setEntry_i32(request, key, 2, values);
                 break;
             }
             case ACAMERA_CONTROL_AF_MODE: {
                 uint8_t mode;
                 if ((mode = focus_mode_string_to_enum(value_s.c_str())) != -1) {
                     ACaptureRequest_setEntry_u8(request, key, 1, &mode);
                 }
                 break;
             }
             case ACAMERA_CONTROL_AF_REGIONS: {
                 std::vector<int32_t> areas;
                 if (parse_areas(value_s, &areas)) {
                     int32_t *values = new int32_t[areas.size()];
                     for (int i = 0; i < areas.size(); i++) {
                         values[i] = areas[i];
                     }
                     ACaptureRequest_setEntry_i32(request, key, areas.size(), values);
                     delete[] values;
                 }
                 break;
             }
             case ACAMERA_CONTROL_AWB_LOCK: {
                 uint8_t value = ACAMERA_CONTROL_AWB_LOCK_OFF;
                 if (!strcmp(value_s.c_str(), android::CameraParameters::TRUE)) {
                     value = ACAMERA_CONTROL_AWB_LOCK_ON;
                 }
                 ACaptureRequest_setEntry_u8(request, key, 1, &value);
                 break;
             }
             case ACAMERA_CONTROL_AWB_MODE: {
                 uint8_t mode;
                 if ((mode = wb_mode_string_to_enum(value_s.c_str())) != -1) {
                     ACaptureRequest_setEntry_u8(request, key, 1, &mode);
                 }
                 break;
             }
             case ACAMERA_CONTROL_EFFECT_MODE: {
                 uint8_t mode;
                 if ((mode = effect_mode_string_to_enum(value_s.c_str())) != -1) {
                     ACaptureRequest_setEntry_u8(request, key, 1, &mode);
                 }
                 break;
             }
             case ACAMERA_CONTROL_SCENE_MODE:
                 uint8_t mode;
                 if ((mode = scene_mode_string_to_enum(value_s.c_str(), ACAMERA_CONTROL_SCENE_MODE_DISABLED)) != -1) {
                     ACaptureRequest_setEntry_u8(request, key, 1, &mode);
                 }
                 break;
             case ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE: {
                uint8_t value = ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE_OFF;
                 if (camera->m_video_mode) {
                     if (!strcmp(value_s.c_str(), android::CameraParameters::TRUE)) {
                         if (request == camera->m_preview_request ||
                             request == camera->m_video_request) {
                             value = ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE_ON;
                         }
                     }
                 }
                 ACaptureRequest_setEntry_u8(request, key, 1, &value);
                 break;
             }
             case ACAMERA_CONTROL_ZOOM_RATIO:
                 if (float value = std::stof(value_s)) {
                     ACaptureRequest_setEntry_float(request, key, 1, &value);
                 }
                 break;
             case ACAMERA_FLASH_MODE: {
                 uint8_t mode;
                 if (!strcmp(value_s.c_str(), android::CameraParameters::FLASH_MODE_TORCH)) {
                     mode = ACAMERA_CONTROL_AE_MODE_ON;
                     ACaptureRequest_setEntry_u8(request, ACAMERA_CONTROL_AE_MODE, 1, &mode);;
                     mode = ACAMERA_FLASH_MODE_TORCH;
                     ACaptureRequest_setEntry_u8(request, ACAMERA_FLASH_MODE, 1, &mode);;
                 } else {
                     if ((mode = flash_mode_string_to_enum(value_s.c_str())) != -1) {
                         ACaptureRequest_setEntry_u8(request, ACAMERA_CONTROL_AE_MODE, 1, &mode);
                     }
                 }
                 break;
             }
             case ACAMERA_JPEG_QUALITY:
                 if (int32_t value = std::stoi(value_s)) {
                    ACaptureRequest_setEntry_u8(camera->m_preview_request, key, 1, &mode);
                 }
                 break;
             default:
                 break;
             }
         }
     }

}

bool droid_media_camera_set_parameters(DroidMediaCamera *camera, const char *params)
{
    ALOGI("set_parameters");
    if (!camera->m_device || !params) {
        return false;
    }

    camera_status_t status;

    int32_t current_height = -1;
    int32_t current_width = -1;
    int32_t current_format = camera->image_format;

    if (camera->m_image_reader) {
        AImageReader_getHeight(camera->m_image_reader, &current_height);
        AImageReader_getWidth(camera->m_image_reader, &current_width);
    }

    std::unordered_map<std::string, std::string> param_map;
    std::istringstream iss(params);
    std::string param;

    while (std::getline(iss, param, ';')) {
        std::string key_s;
        std::string value_s;
        parse_pair_string(param, '=', key_s, value_s);
        param_map.insert({key_s, value_s});

        ALOGI("set_parameters %s=%s", key_s.c_str(), value_s.c_str());

        if (param_key_string_to_enum(key_s.c_str()) == -1) {
            if (!strcmp(key_s.c_str(), "picture-format")) {
                //TODO is this needed?
            } else if (!strcmp(key_s.c_str(), "picture-size")) {
                parse_pair_int32(value_s, 'x', camera->image_width, camera->image_height);
            } else if (!strcmp(key_s.c_str(), "preview-size")) {
                parse_pair_int32(value_s, 'x', camera->preview_width, camera->preview_height);
            } else if (!strcmp(key_s.c_str(), "video-size")) {
                parse_pair_int32(value_s, 'x', camera->video_width, camera->video_height);
            }
        }
    }

    if (!camera->m_image_reader || current_width != camera->image_width ||
        current_height != camera->image_height || current_format != camera->image_format) {
        ALOGI("Updating image reader");
        setup_image_reader(camera);
    }

    if (!camera->m_video_mode) {
        update_request(camera, camera->m_image_request, param_map);
    } else {
        update_request(camera, camera->m_video_request, param_map);
    }

    if (camera->m_preview_request) {
        update_request(camera, camera->m_preview_request, param_map);
        if (camera->m_preview_enabled) {
            ALOGI("Updating viewfinder");
            status = ACameraCaptureSession_setRepeatingRequest(camera->m_session, &camera->m_capture_callbacks, 1,
                &camera->m_preview_request, NULL);
            if (status != ACAMERA_OK) {
                ALOGI("Updating preview failed");
                return false;
            }
        }
    }

    return true;
}

char *droid_media_camera_get_parameters(DroidMediaCamera *camera)
{
    ALOGI("get_parameters");
    camera_status_t status;
    std::string params;
    int32_t numEntries;
    const uint32_t *tags;

    if (!camera->m_device || !camera->m_metadata) {
        return NULL;
    }

    status = ACameraMetadata_getAllTags(camera->m_metadata, &numEntries, &tags);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to all tags from camera (status: %d)\n", status);
        return NULL;
    }

    if (camera->image_height != -1 && camera->image_width != -1) {
        params += "picture-size="+std::to_string(camera->image_width)+"x"+std::to_string(camera->image_height)+";";
    }

    if (camera->preview_height != -1 && camera->preview_width != -1) {
        params += "preview-size="+std::to_string(camera->preview_width)+"x"+std::to_string(camera->preview_height)+";";
    }

    if (camera->video_height != -1 && camera->video_width != -1) {
        params += "video-size="+std::to_string(camera->video_width)+"x"+std::to_string(camera->video_height)+";";
    }

    for (int32_t i = 0; i < numEntries; i++) {
        ACameraMetadata_const_entry entry;
        status = ACameraMetadata_getConstEntry(camera->m_metadata, tags[i], &entry);
        if (status != ACAMERA_OK) {
            ALOGI("Failed to get entry from camera (status: %d)\n", status);
            continue;
        }

        switch (tags[i]) {
        case ACAMERA_CONTROL_AE_AVAILABLE_ANTIBANDING_MODES:
            if (entry.count > 0) {
                bool found = false;
                std::string ab = "antibanding-values=";
                for (int32_t j = 0; j < entry.count; j++) {
                    if (found)
                        ab += ",";
                    ab += ab_mode_enum_to_string(entry.data.u8[j], found);
                }
                params += ab + ";";
            }
            break;
        case ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES: {
            if (entry.count > 0) {
                std::string fps = "preview-fps-range-values=";
                for (int32_t j = 0; j < entry.count; j = j + 2) {
                    if (j > 0)
                        fps += ",";
                    fps += "(" + std::to_string(entry.data.i32[j]);
                    fps += ",";
                    fps += std::to_string(entry.data.i32[j+1]) + ")";
                }
                params += fps + ";";
                params += "preview-frame-rate=30;";
            }
            break;
        }
        case ACAMERA_CONTROL_AE_COMPENSATION_RANGE:
            params += "min-exposure-compensation="+std::to_string(entry.data.i32[0])+";";
            params += "max-exposure-compensation="+std::to_string(entry.data.i32[1])+";";
            break;
        case ACAMERA_CONTROL_AE_COMPENSATION_STEP:
            params += "exposure-compensation-step="+std::to_string((float)entry.data.r[0].numerator / (float)entry.data.r[0].denominator)+";";
            break;
        case ACAMERA_CONTROL_AE_LOCK_AVAILABLE:
            if (entry.count > 0 || entry.data.u8[0] == ACAMERA_CONTROL_AE_LOCK_AVAILABLE_TRUE) {
                params += "auto-exposure-lock-supported=true;";
                params += "auto-exposure-lock=false;";
            } else {
                params += "auto-exposure-lock-supported=false;";
            }
            break;
        case ACAMERA_CONTROL_AF_AVAILABLE_MODES:
            if (entry.count > 0) {
                bool found = false;
                std::string af_modes = "focus-mode-values=";
                for (int32_t j = 0; j < entry.count; j++) {
                    if (found)
                        af_modes += ",";
                    bool fixed_lens = false;
                    ACameraMetadata_const_entry min_focus_distance_entry;
                    status = ACameraMetadata_getConstEntry(camera->m_metadata,
                        ACAMERA_LENS_INFO_MINIMUM_FOCUS_DISTANCE, &min_focus_distance_entry);
                    if (status == ACAMERA_OK) {
                        fixed_lens = min_focus_distance_entry.count == 0 ||
                            min_focus_distance_entry.data.f[0] == 0;
                    }
                    af_modes += focus_mode_enum_to_string(entry.data.u8[j], fixed_lens, found);
                }
                params += af_modes + ";";
            }
            break;
        case ACAMERA_CONTROL_AVAILABLE_SCENE_MODES:
            if (entry.count > 0) {
                bool found = false;
                std::string scene_modes = "scene-mode-values=";
                for (int32_t j = 0; j < entry.count; j++) {
                    if (found)
                        scene_modes += ",";
                    scene_modes += scene_mode_enum_to_string(entry.data.u8[j], found);
                }
                params += scene_modes + ";";
            }
            break;
        case ACAMERA_CONTROL_AVAILABLE_EFFECTS:
            if (entry.count > 0) {
                bool found = false;
                std::string effects = "effect-values=";
                for (int32_t j = 0; j < entry.count; j++) {
                    if (found)
                        effects += ",";
                    effects += effect_mode_enum_to_string(entry.data.u8[j], found);
                }
                params += effects + ";";
            }
            break;
        case ACAMERA_CONTROL_AWB_AVAILABLE_MODES:
            if (entry.count > 0) {
                bool found = false;
                std::string wb = "whitebalance-values=";
                for (int32_t j = 0; j < entry.count; j++) {
                    if (found)
                        wb += ",";
                    wb += wb_mode_enum_to_string(entry.data.u8[j], found);
                }
                params += wb + ";";
            }
            break;
        case ACAMERA_CONTROL_AWB_LOCK_AVAILABLE:
            if (entry.count > 0 || entry.data.u8[0] == ACAMERA_CONTROL_AWB_LOCK_AVAILABLE_TRUE) {
                params += "auto-whitebalance-lock-supported=true;";
                params += "auto-whitebalance-lock=false;";
            } else {
                params += "auto-whitebalance-lock-supported=false;";
            }
            break;
        case ACAMERA_CONTROL_MAX_REGIONS:
            if (entry.count > 0) {
                 params += "max-num-focus-areas"+std::to_string(entry.data.u8[2])+";";
                 params += "max-num-metering-areas"+std::to_string(entry.data.u8[0])+";";
                 camera->max_ae_regions = entry.data.u8[0];
                 camera->max_awb_regions = entry.data.u8[1];
                 camera->max_focus_regions = entry.data.u8[2];
            }
            break;
        case ACAMERA_CONTROL_VIDEO_STABILIZATION_MODE:
            if (entry.count > 1) {
                 ALOGI("video stabilization supported");
                 params += "video-stabilization-supported = true;";
            }
            break;
        case ACAMERA_CONTROL_ZOOM_RATIO_RANGE:
            params += "max-zoom="+std::to_string(entry.data.f[1])+";";
            break;
        case ACAMERA_FLASH_INFO_AVAILABLE:
            if (entry.data.u8[0] == ACAMERA_FLASH_INFO_AVAILABLE_FALSE) {
                params += "flash-mode-values=off;";
            } else {
                params += "flash-mode-values=off,auto,on,torch;";
            }
            break;
        case ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS: {
            std::string image = "picture-size-values=";
            std::string preview = "preview-size-values=";
            std::string video = "video-size-values=";

            //TODO Verify correct stream configurations for each case
            std::set<int32_t> formats;
            for (int32_t j = 0; j < entry.count; j = j + 4) {
                formats.insert(entry.data.i32[j]);

                switch (entry.data.i32[j]) {
                case AIMAGE_FORMAT_JPEG:
                    if (image.length() > 20)
                        image += ",";
                    image += std::to_string(entry.data.i32[j+1]);
                    image += "x";
                    image += std::to_string(entry.data.i32[j+2]);
                    break;
/*
                case AIMAGE_FORMAT_YUV_420_888:
                    if (preview.length() > 20)
                        preview += ",";
                    preview += std::to_string(entry.data.i32[j+1]);
                    preview += "x";
                    preview += std::to_string(entry.data.i32[j+2]);
                    break;
*/
                case AIMAGE_FORMAT_PRIVATE:
                    if (video.length() > 18)
                        video += ",";
                    video += std::to_string(entry.data.i32[j+1]);
                    video += "x";
                    video += std::to_string(entry.data.i32[j+2]);
                    if (preview.length() > 20)
                        preview += ",";
                    preview += std::to_string(entry.data.i32[j+1]);
                    preview += "x";
                    preview += std::to_string(entry.data.i32[j+2]);
                    break;
                default:
                    break;
                }
            }
            params += image + ";";
            params += preview + ";";
            params += video + ";";

            std::set<int32_t>::iterator itr;
            for (itr = formats.begin(); itr != formats.end(); itr++) {
                ALOGI("format %i %x", *itr, *itr);
            }
            break;
        }
        default:
            break;
        }
    }

    ALOGI("get_parameters result: %s", params.c_str());
    size_t len = params.length();

    char *c_params = (char *)malloc(len + 1);
    if (!c_params) {
        ALOGE("Failed to allocate enough memory for camera parameters");
        return NULL;
    }

    memcpy(c_params, params.c_str(), len);
    c_params[len] = '\0';

    return c_params;
}

bool droid_media_camera_take_picture(DroidMediaCamera *camera, int msgType)
{
    int seq_id;
    camera_status_t status;
    ALOGI("take_picture");

    if (!camera->m_session) {
        return false;
    }

    status = ACameraCaptureSession_capture(camera->m_session, &camera->m_capture_callbacks, 1,
        &camera->m_image_request, &seq_id);

    return status == ACAMERA_OK;
}

void droid_media_camera_release_recording_frame(DroidMediaCamera *camera, DroidMediaCameraRecordingData *data)
{
    // TODO release recording frame
}

nsecs_t droid_media_camera_recording_frame_get_timestamp(DroidMediaCameraRecordingData *data)
{
    // TODO recording get frame timestamp
    return 0;
}

size_t droid_media_camera_recording_frame_get_size(DroidMediaCameraRecordingData *data)
{
    // TODO recording get frame size
    return 0;
}

void *droid_media_camera_recording_frame_get_data(DroidMediaCameraRecordingData *data)
{
    // TODO recording get frame data
    return NULL;
}

bool droid_media_camera_enable_face_detection(DroidMediaCamera *camera,
                                              DroidMediaCameraFaceDetectionType type,
                                              bool enable)
{
    ALOGI("enable_face_detection enable: %i", enable);
    camera_status_t status;
    uint8_t faceDetectMode = enable ? ACAMERA_STATISTICS_FACE_DETECT_MODE_SIMPLE
                                    : ACAMERA_STATISTICS_FACE_DETECT_MODE_OFF;
    status = ACaptureRequest_setEntry_u8(camera->m_preview_request,
        ACAMERA_STATISTICS_FACE_DETECT_MODE, 1, &faceDetectMode);

    return status == ACAMERA_OK;
}

int32_t droid_media_camera_get_video_color_format(DroidMediaCamera *camera)
{
    // TODO get video color format
    ALOGI("get_video_color_format");
    //return AIMAGE_FORMAT_YUV_420_888;
    return AIMAGE_FORMAT_PRIVATE;
}

};

android::sp<android::Camera> droid_media_camera_get_camera(DroidMediaCamera *camera)
{
    // TODO Is this needed with camera2?
    return NULL;
}

#endif
