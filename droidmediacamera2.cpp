/*
 * Copyright (C) 2014-2015 Jolla Ltd.
 * Copyright (C) 2026 Jolla Mobile Ltd
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
 * Camera2 NDK implementation of the DroidMediaCamera API.
 *
 * The Camera1 style API (parameters string and callbacks) is emulated on top
 * of a capture session. So far the session has a single output, the preview
 * buffer queue returned by droid_media_camera_get_buffer_queue. Still capture,
 * preview frame callbacks, autofocus and recording are not implemented yet.
 */

#include "droidmediacamera.h"
#include "private.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <algorithm>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <android/log.h>
#include <camera/NdkCameraManager.h>
#include <camera/NdkCameraDevice.h>
#include <camera/NdkCameraCaptureSession.h>
#include <camera/NdkCaptureRequest.h>
#include <camera/NdkCameraMetadata.h>
#include <media/openmax/OMX_IVCommon.h>
#include <system/camera.h>
#include <system/graphics.h>
#include <gui/Surface.h>
#include <utils/Mutex.h>

#undef LOG_TAG
#define LOG_TAG "DroidMediaCamera2"

namespace {

// Camera1 limits the preview to 1080p
const int32_t MAX_PREVIEW_WIDTH = 1920;
const int32_t MAX_PREVIEW_HEIGHT = 1080;

struct Size {
    int32_t width;
    int32_t height;

    bool operator==(const Size &other) const {
        return width == other.width && height == other.height;
    }
    bool operator!=(const Size &other) const { return !(*this == other); }
};

struct FpsRange {
    int32_t min;
    int32_t max;
};

bool contains(const std::vector<Size> &list, const Size &size)
{
    return std::find(list.begin(), list.end(), size) != list.end();
}

std::string size_to_string(const Size &size)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%dx%d", size.width, size.height);
    return buf;
}

std::string sizes_to_string(const std::vector<Size> &sizes)
{
    std::string str;
    for (size_t i = 0; i < sizes.size(); ++i) {
        if (i) {
            str += ",";
        }
        str += size_to_string(sizes[i]);
    }
    return str;
}

bool parse_size(const std::string &str, Size *size)
{
    return sscanf(str.c_str(), "%dx%d", &size->width, &size->height) == 2
        && size->width > 0 && size->height > 0;
}

std::string int_to_string(int64_t value)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%lld", (long long)value);
    return buf;
}

bool size_area_greater(const Size &a, const Size &b)
{
    return (int64_t)a.width * a.height > (int64_t)b.width * b.height;
}

android::Mutex g_manager_lock;
ACameraManager *g_manager = NULL;

ACameraManager *camera_manager()
{
    android::Mutex::Autolock locker(g_manager_lock);
    if (!g_manager) {
        g_manager = ACameraManager_create();
    }
    return g_manager;
}

bool camera_id_less(const std::string &a, const std::string &b)
{
    // Camera ids are usually numbers, keep "10" after "2"
    char *end_a, *end_b;
    long num_a = strtol(a.c_str(), &end_a, 10);
    long num_b = strtol(b.c_str(), &end_b, 10);
    bool numeric_a = !a.empty() && *end_a == '\0';
    bool numeric_b = !b.empty() && *end_b == '\0';
    if (numeric_a && numeric_b) {
        return num_a < num_b;
    }
    if (numeric_a != numeric_b) {
        return numeric_a;
    }
    return a < b;
}

std::vector<std::string> camera_ids()
{
    std::vector<std::string> ids;
    ACameraManager *manager = camera_manager();
    ACameraIdList *list = NULL;

    if (!manager || ACameraManager_getCameraIdList(manager, &list) != ACAMERA_OK || !list) {
        ALOGE("Failed to get the camera id list");
        return ids;
    }

    for (int i = 0; i < list->numCameras; ++i) {
        ids.push_back(list->cameraIds[i]);
    }
    ACameraManager_deleteCameraIdList(list);

    std::sort(ids.begin(), ids.end(), camera_id_less);
    return ids;
}

ACameraMetadata *camera_characteristics(int camera_number, std::string *id)
{
    std::vector<std::string> ids = camera_ids();
    if (camera_number < 0 || camera_number >= (int)ids.size()) {
        ALOGE("No camera %d", camera_number);
        return NULL;
    }

    ACameraMetadata *chars = NULL;
    if (ACameraManager_getCameraCharacteristics(camera_manager(), ids[camera_number].c_str(),
                                                &chars) != ACAMERA_OK) {
        ALOGE("Failed to get characteristics of camera %s", ids[camera_number].c_str());
        return NULL;
    }

    if (id) {
        *id = ids[camera_number];
    }
    return chars;
}

std::vector<int32_t> metadata_i32(const ACameraMetadata *metadata, uint32_t tag)
{
    ACameraMetadata_const_entry entry;
    if (ACameraMetadata_getConstEntry(metadata, tag, &entry) != ACAMERA_OK) {
        return std::vector<int32_t>();
    }
    return std::vector<int32_t>(entry.data.i32, entry.data.i32 + entry.count);
}

bool metadata_first_u8(const ACameraMetadata *metadata, uint32_t tag, uint8_t *value)
{
    ACameraMetadata_const_entry entry;
    if (ACameraMetadata_getConstEntry(metadata, tag, &entry) != ACAMERA_OK || entry.count < 1) {
        return false;
    }
    *value = entry.data.u8[0];
    return true;
}

std::vector<Size> output_sizes(const ACameraMetadata *chars, int32_t format)
{
    std::vector<Size> sizes;
    std::vector<int32_t> configs = metadata_i32(chars, ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS);

    for (size_t i = 0; i + 3 < configs.size(); i += 4) {
        if (configs[i] == format
                && configs[i + 3] == ACAMERA_SCALER_AVAILABLE_STREAM_CONFIGURATIONS_OUTPUT) {
            Size size = { configs[i + 1], configs[i + 2] };
            if (!contains(sizes, size)) {
                sizes.push_back(size);
            }
        }
    }

    std::sort(sizes.begin(), sizes.end(), size_area_greater);
    return sizes;
}

} // namespace

extern "C" {

struct _DroidMediaCameraRecordingData
{
    // Raw recording frames are not supported by the Camera2 backend
    nsecs_t ts;
};

struct _DroidMediaCamera
{
    _DroidMediaCamera();

    bool open(int camera_number);
    void close();

    void initStaticInfo();
    void initParameters();
    void setStatic(const char *key, const std::string &value);
    void setDefault(const char *key, const std::string &value);
    std::string param(const char *key) const;
    bool setParameters(const char *params);
    char *flattenParameters() const;

    Size previewSize() const;
    bool ensureSession();
    void destroySession();
    ACaptureRequest *buildRequest();
    bool updateRepeating();

    bool startPreview();
    void stopPreview();

    void deviceError(int error);

    // Device, session and outputs. Guarded by m_lock, which is only taken by API calls.
    android::Mutex m_lock;
    std::string m_id;
    ACameraMetadata *m_chars;
    ACameraDevice *m_device;
    ACameraCaptureSession *m_session;
    ACaptureSessionOutputContainer *m_container;
    ACaptureSessionOutput *m_previewOutput;
    Size m_sessionPreviewSize;

    android::sp<DroidMediaBufferQueue> m_queue;
    android::sp<android::Surface> m_previewWindow;
    ACameraOutputTarget *m_previewTarget;

    bool m_previewRunning;

    // Camera1 parameters
    std::map<std::string, std::string> m_params;
    std::set<std::string> m_readOnly;

    // Static camera information
    std::vector<Size> m_previewSizes;
    std::vector<Size> m_videoSizes;
    std::vector<Size> m_pictureSizes;
    std::vector<FpsRange> m_fpsRanges;

    // Shared with the NDK callback threads
    android::Mutex m_stateLock;
    bool m_closing;

    DroidMediaCameraCallbacks m_cb;
    void *m_cb_data;

    ACameraDevice_StateCallbacks m_deviceCallbacks;
    ACameraCaptureSession_stateCallbacks m_sessionCallbacks;
};

}

namespace {

void device_disconnected(void *context, ACameraDevice *device)
{
    static_cast<DroidMediaCamera *>(context)->deviceError(-1);
}

void device_error(void *context, ACameraDevice *device, int error)
{
    static_cast<DroidMediaCamera *>(context)->deviceError(error);
}

void session_closed(void *context, ACameraCaptureSession *session)
{
    ALOGD("Capture session %p closed", session);
}

void session_ready(void *context, ACameraCaptureSession *session)
{
}

void session_active(void *context, ACameraCaptureSession *session)
{
}

} // namespace

_DroidMediaCamera::_DroidMediaCamera() :
    m_chars(NULL),
    m_device(NULL),
    m_session(NULL),
    m_container(NULL),
    m_previewOutput(NULL),
    m_previewTarget(NULL),
    m_previewRunning(false),
    m_closing(false),
    m_cb_data(0)
{
    memset(&m_cb, 0x0, sizeof(m_cb));
    m_sessionPreviewSize = Size{0, 0};

    m_deviceCallbacks.context = this;
    m_deviceCallbacks.onDisconnected = device_disconnected;
    m_deviceCallbacks.onError = device_error;

    m_sessionCallbacks.context = this;
    m_sessionCallbacks.onClosed = session_closed;
    m_sessionCallbacks.onReady = session_ready;
    m_sessionCallbacks.onActive = session_active;
}

bool _DroidMediaCamera::open(int camera_number)
{
    m_chars = camera_characteristics(camera_number, &m_id);
    if (!m_chars) {
        return false;
    }

    initStaticInfo();
    initParameters();

    m_queue = new DroidMediaBufferQueue("DroidMediaCameraBufferQueue");
    if (!m_queue->connectListener()) {
        ALOGE("Failed to connect buffer queue listener");
        return false;
    }
    // A preview stream, like the one Camera1 sets up. The size is set from
    // preview-size when the session is created.
    m_queue->setDefaultBufferFormat(HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);
    m_previewWindow = static_cast<android::Surface *>(m_queue->window());
    if (ACameraOutputTarget_create(m_previewWindow.get(), &m_previewTarget) != ACAMERA_OK) {
        ALOGE("Failed to create the preview output target");
        return false;
    }

    camera_status_t status = ACameraManager_openCamera(camera_manager(), m_id.c_str(),
                                                       &m_deviceCallbacks, &m_device);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to open camera %s: %d", m_id.c_str(), status);
        m_device = NULL;
        return false;
    }

    ALOGI("Opened camera %s", m_id.c_str());
    return true;
}

void _DroidMediaCamera::close()
{
    android::Mutex::Autolock locker(m_lock);
    {
        android::Mutex::Autolock state(m_stateLock);
        m_closing = true;
    }

    destroySession();

    if (m_device) {
        ACameraDevice_close(m_device);
        m_device = NULL;
    }

    if (m_previewTarget) {
        ACameraOutputTarget_free(m_previewTarget);
        m_previewTarget = NULL;
    }

    if (m_queue.get()) {
        m_queue->setCallbacks(0, 0);
    }
    m_previewWindow.clear();

    if (m_chars) {
        ACameraMetadata_free(m_chars);
        m_chars = NULL;
    }
}

void _DroidMediaCamera::initStaticInfo()
{
    std::vector<Size> priv = output_sizes(m_chars, HAL_PIXEL_FORMAT_IMPLEMENTATION_DEFINED);

    for (size_t i = 0; i < priv.size(); ++i) {
        if (priv[i].width <= MAX_PREVIEW_WIDTH && priv[i].height <= MAX_PREVIEW_HEIGHT) {
            m_previewSizes.push_back(priv[i]);
        }
    }
    m_videoSizes = priv;
    m_pictureSizes = output_sizes(m_chars, HAL_PIXEL_FORMAT_BLOB);

    std::vector<int32_t> fps = metadata_i32(m_chars, ACAMERA_CONTROL_AE_AVAILABLE_TARGET_FPS_RANGES);
    for (size_t i = 0; i + 1 < fps.size(); i += 2) {
        FpsRange range = { fps[i], fps[i + 1] };
        m_fpsRanges.push_back(range);
    }
}

void _DroidMediaCamera::setStatic(const char *key, const std::string &value)
{
    m_params[key] = value;
    m_readOnly.insert(key);
}

void _DroidMediaCamera::setDefault(const char *key, const std::string &value)
{
    m_params[key] = value;
}

std::string _DroidMediaCamera::param(const char *key) const
{
    std::map<std::string, std::string>::const_iterator it = m_params.find(key);
    return it == m_params.end() ? std::string() : it->second;
}

void _DroidMediaCamera::initParameters()
{
    // Sizes
    setStatic("preview-size-values", sizes_to_string(m_previewSizes));
    setStatic("video-size-values", sizes_to_string(m_videoSizes));
    setStatic("picture-size-values", sizes_to_string(m_pictureSizes));

    Size picture = m_pictureSizes.empty() ? Size{640, 480} : m_pictureSizes[0];
    Size preview = m_previewSizes.empty() ? Size{640, 480} : m_previewSizes[0];
    // The largest preview with the aspect ratio of the largest picture
    for (size_t i = 0; i < m_previewSizes.size(); ++i) {
        if ((int64_t)m_previewSizes[i].width * picture.height
                == (int64_t)m_previewSizes[i].height * picture.width) {
            preview = m_previewSizes[i];
            break;
        }
    }
    Size video = preview;
    Size full_hd = { 1920, 1080 };
    if (contains(m_videoSizes, full_hd)) {
        video = full_hd;
    }

    setDefault("preview-size", size_to_string(preview));
    setDefault("picture-size", size_to_string(picture));
    setDefault("video-size", size_to_string(video));
    setStatic("preferred-preview-size-for-video", size_to_string(preview));

    // Formats
    setStatic("preview-format-values", "yuv420sp");
    setDefault("preview-format", "yuv420sp");
    setStatic("picture-format-values", "jpeg");
    setDefault("picture-format", "jpeg");
    setStatic("video-frame-format", "yuv420sp");

    // Frame rates, in fps * 1000 like Camera1
    std::string ranges;
    std::set<int32_t> rates;
    int32_t frame_rate = 0;
    for (size_t i = 0; i < m_fpsRanges.size(); ++i) {
        if (!ranges.empty()) {
            ranges += ",";
        }
        ranges += "(" + int_to_string(m_fpsRanges[i].min * 1000) + ","
            + int_to_string(m_fpsRanges[i].max * 1000) + ")";
        rates.insert(m_fpsRanges[i].max);
        if (m_fpsRanges[i].max <= 30) {
            frame_rate = std::max(frame_rate, m_fpsRanges[i].max);
        }
    }
    if (!frame_rate) {
        frame_rate = rates.empty() ? 30 : *rates.begin();
    }
    // The widest range ending at the default frame rate
    FpsRange preview_range = { frame_rate, frame_rate };
    for (size_t i = 0; i < m_fpsRanges.size(); ++i) {
        if (m_fpsRanges[i].max == frame_rate && m_fpsRanges[i].min < preview_range.min) {
            preview_range = m_fpsRanges[i];
        }
    }
    std::string rate_values;
    for (std::set<int32_t>::const_iterator it = rates.begin(); it != rates.end(); ++it) {
        if (!rate_values.empty()) {
            rate_values += ",";
        }
        rate_values += int_to_string(*it);
    }
    setStatic("preview-fps-range-values", ranges);
    setDefault("preview-fps-range", int_to_string(preview_range.min * 1000) + ","
               + int_to_string(preview_range.max * 1000));
    setStatic("preview-frame-rate-values", rate_values);
    setDefault("preview-frame-rate", int_to_string(frame_rate));

    // Not implemented yet: the camera runs with its template defaults
    setStatic("focus-mode-values", "fixed");
    setDefault("focus-mode", "fixed");
    setStatic("flash-mode-values", "off");
    setDefault("flash-mode", "off");
    setStatic("zoom-supported", "false");
    setStatic("max-zoom", "0");
    setDefault("zoom", "0");
    setStatic("video-snapshot-supported", "false");
}

bool _DroidMediaCamera::setParameters(const char *params)
{
    std::map<std::string, std::string> values = m_params;
    std::string str(params ? params : "");
    std::string::size_type start = 0;

    while (start < str.size()) {
        std::string::size_type end = str.find(';', start);
        if (end == std::string::npos) {
            end = str.size();
        }
        std::string item = str.substr(start, end - start);
        start = end + 1;

        std::string::size_type eq = item.find('=');
        if (eq == std::string::npos || eq == 0) {
            continue;
        }
        std::string key = item.substr(0, eq);
        if (!m_readOnly.count(key)) {
            values[key] = item.substr(eq + 1);
        }
    }

    Size size;
    if (!parse_size(values["preview-size"], &size) || !contains(m_previewSizes, size)) {
        ALOGE("Unsupported preview-size %s", values["preview-size"].c_str());
        return false;
    }

    int min_fps, max_fps;
    if (sscanf(values["preview-fps-range"].c_str(), "%d,%d", &min_fps, &max_fps) == 2) {
        bool found = false;
        for (size_t i = 0; i < m_fpsRanges.size(); ++i) {
            if (m_fpsRanges[i].min * 1000 == min_fps && m_fpsRanges[i].max * 1000 == max_fps) {
                found = true;
                break;
            }
        }
        if (!found) {
            ALOGE("Unsupported preview-fps-range %s", values["preview-fps-range"].c_str());
            return false;
        }
    }

    m_params = values;

    // A new preview size is applied when the preview is started again
    return updateRepeating();
}

char *_DroidMediaCamera::flattenParameters() const
{
    std::string str;
    for (std::map<std::string, std::string>::const_iterator it = m_params.begin();
         it != m_params.end(); ++it) {
        if (!str.empty()) {
            str += ";";
        }
        str += it->first + "=" + it->second;
    }

    char *params = (char *)malloc(str.size() + 1);
    if (!params) {
        ALOGE("Failed to allocate enough memory for camera parameters");
        return NULL;
    }
    memcpy(params, str.c_str(), str.size() + 1);
    return params;
}

Size _DroidMediaCamera::previewSize() const
{
    Size size = { 640, 480 };
    parse_size(param("preview-size"), &size);
    return size;
}

bool _DroidMediaCamera::ensureSession()
{
    Size preview = previewSize();

    if (m_session && m_sessionPreviewSize == preview) {
        return true;
    }

    destroySession();

    // A buffer queue consumer reports this as its size to the camera
    m_queue->setDefaultBufferSize(preview.width, preview.height);

    if (ACaptureSessionOutputContainer_create(&m_container) != ACAMERA_OK) {
        ALOGE("Failed to create a session output container");
        m_container = NULL;
        return false;
    }

    if (ACaptureSessionOutput_create(m_previewWindow.get(), &m_previewOutput) != ACAMERA_OK
            || ACaptureSessionOutputContainer_add(m_container, m_previewOutput) != ACAMERA_OK) {
        ALOGE("Failed to add the preview session output");
        destroySession();
        return false;
    }

    camera_status_t status = ACameraDevice_createCaptureSession(m_device, m_container,
                                                                &m_sessionCallbacks, &m_session);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to create capture session: %d", status);
        m_session = NULL;
        destroySession();
        return false;
    }

    ALOGI("Created capture session with a %dx%d preview", preview.width, preview.height);
    m_sessionPreviewSize = preview;
    return true;
}

void _DroidMediaCamera::destroySession()
{
    if (m_session) {
        ACameraCaptureSession_close(m_session);
        m_session = NULL;
    }
    if (m_previewOutput) {
        ACaptureSessionOutput_free(m_previewOutput);
        m_previewOutput = NULL;
    }
    if (m_container) {
        ACaptureSessionOutputContainer_free(m_container);
        m_container = NULL;
    }
    m_sessionPreviewSize = Size{0, 0};
}

ACaptureRequest *_DroidMediaCamera::buildRequest()
{
    ACaptureRequest *request = NULL;
    if (ACameraDevice_createCaptureRequest(m_device, TEMPLATE_PREVIEW, &request) != ACAMERA_OK) {
        ALOGE("Failed to create a preview capture request");
        return NULL;
    }
    ACaptureRequest_addTarget(request, m_previewTarget);

    int32_t range[2];
    if (sscanf(param("preview-fps-range").c_str(), "%d,%d", &range[0], &range[1]) == 2) {
        range[0] /= 1000;
        range[1] /= 1000;
        ACaptureRequest_setEntry_i32(request, ACAMERA_CONTROL_AE_TARGET_FPS_RANGE, 2, range);
    }

    return request;
}

bool _DroidMediaCamera::updateRepeating()
{
    if (!m_previewRunning || !m_session) {
        return true;
    }

    ACaptureRequest *request = buildRequest();
    if (!request) {
        return false;
    }

    camera_status_t status = ACameraCaptureSession_setRepeatingRequest(m_session, NULL, 1,
                                                                      &request, NULL);
    ACaptureRequest_free(request);
    if (status != ACAMERA_OK) {
        ALOGE("Failed to set the repeating request: %d", status);
        return false;
    }

    return true;
}

bool _DroidMediaCamera::startPreview()
{
    if (m_previewRunning) {
        return true;
    }

    if (!ensureSession()) {
        return false;
    }

    m_previewRunning = true;
    if (!updateRepeating()) {
        m_previewRunning = false;
        return false;
    }

    return true;
}

void _DroidMediaCamera::stopPreview()
{
    if (!m_previewRunning) {
        return;
    }

    m_previewRunning = false;
    if (m_session) {
        ACameraCaptureSession_stopRepeating(m_session);
    }
}

void _DroidMediaCamera::deviceError(int error)
{
    {
        android::Mutex::Autolock locker(m_stateLock);
        if (m_closing) {
            return;
        }
    }

    int camera_error = CAMERA_ERROR_UNKNOWN;
    if (error < 0) {
        ALOGE("Camera %s disconnected", m_id.c_str());
        camera_error = CAMERA_ERROR_RELEASED;
    } else {
        ALOGE("Camera %s error %d", m_id.c_str(), error);
        if (error == ERROR_CAMERA_SERVICE) {
            camera_error = CAMERA_ERROR_SERVER_DIED;
        }
    }

    if (m_cb.error_cb) {
        m_cb.error_cb(m_cb_data, camera_error);
    }
}

extern "C" {

DroidMediaBufferQueue *droid_media_camera_get_buffer_queue (DroidMediaCamera *camera)
{
    return camera->m_queue.get();
}

DroidMediaBufferQueue *droid_media_camera_get_recording_buffer_queue (DroidMediaCamera *camera)
{
    return NULL;
}

int droid_media_camera_get_number_of_cameras()
{
    return camera_ids().size();
}

bool droid_media_camera_get_info(DroidMediaCameraInfo *info, int camera_number)
{
    ACameraMetadata *chars = camera_characteristics(camera_number, NULL);
    if (!chars) {
        return false;
    }

    uint8_t facing = ACAMERA_LENS_FACING_BACK;
    metadata_first_u8(chars, ACAMERA_LENS_FACING, &facing);
    std::vector<int32_t> orientation = metadata_i32(chars, ACAMERA_SENSOR_ORIENTATION);
    ACameraMetadata_free(chars);

    info->facing = facing == ACAMERA_LENS_FACING_FRONT
        ? DROID_MEDIA_CAMERA_FACING_FRONT : DROID_MEDIA_CAMERA_FACING_BACK;
    info->orientation = orientation.empty() ? 0 : orientation[0];
    return true;
}

bool droid_media_camera_set_torch_mode(bool enabled)
{
    ALOGE("Torch is not supported yet");
    return false;
}

DroidMediaCamera *droid_media_camera_connect(int camera_number)
{
    DroidMediaCamera *cam = new DroidMediaCamera;
    if (!cam) {
        ALOGE("Failed to allocate DroidMediaCamera");
        return NULL;
    }

    if (!cam->open(camera_number)) {
        ALOGE("Failed to connect to camera %d", camera_number);
        cam->close();
        delete cam;
        return NULL;
    }

    return cam;
}

bool droid_media_camera_reconnect(DroidMediaCamera *camera)
{
    // The device stays open for as long as the camera is connected
    return camera->m_device != NULL;
}

void droid_media_camera_disconnect(DroidMediaCamera *camera)
{
    camera->close();
    delete camera;
}

bool droid_media_camera_lock(DroidMediaCamera *camera)
{
    return true;
}

bool droid_media_camera_unlock(DroidMediaCamera *camera)
{
    return true;
}

bool droid_media_camera_start_preview(DroidMediaCamera *camera)
{
    android::Mutex::Autolock locker(camera->m_lock);
    return camera->startPreview();
}

void droid_media_camera_stop_preview(DroidMediaCamera *camera)
{
    android::Mutex::Autolock locker(camera->m_lock);
    camera->stopPreview();
}

bool droid_media_camera_is_preview_enabled(DroidMediaCamera *camera)
{
    android::Mutex::Autolock locker(camera->m_lock);
    return camera->m_previewRunning;
}

bool droid_media_camera_start_recording(DroidMediaCamera *camera)
{
    ALOGE("Recording is not supported yet");
    return false;
}

void droid_media_camera_stop_recording(DroidMediaCamera *camera)
{
}

bool droid_media_camera_is_recording_enabled(DroidMediaCamera *camera)
{
    return false;
}

bool droid_media_camera_start_auto_focus(DroidMediaCamera *camera)
{
    // Fixed focus, Camera1 reports success right away
    if (camera->m_cb.focus_cb) {
        camera->m_cb.focus_cb(camera->m_cb_data, 1);
    }
    return true;
}

bool droid_media_camera_cancel_auto_focus(DroidMediaCamera *camera)
{
    return true;
}

void droid_media_camera_set_callbacks(DroidMediaCamera *camera, DroidMediaCameraCallbacks *cb, void *data)
{
    memcpy(&camera->m_cb, cb, sizeof(camera->m_cb));
    camera->m_cb_data = data;
}

bool droid_media_camera_send_command(DroidMediaCamera *camera, int32_t cmd, int32_t arg1, int32_t arg2)
{
    switch (cmd) {
    case CAMERA_CMD_ENABLE_SHUTTER_SOUND:
    case CAMERA_CMD_PLAY_RECORDING_SOUND:
    case CAMERA_CMD_SET_DISPLAY_ORIENTATION:
    case CAMERA_CMD_PING:
        // Sounds and display orientation are up to the client
        return true;

    default:
        ALOGW("Unsupported command %d (%d, %d)", cmd, arg1, arg2);
        return false;
    }
}

bool droid_media_camera_store_meta_data_in_buffers(DroidMediaCamera *camera, bool enabled)
{
    return !enabled;
}

void droid_media_camera_set_preview_callback_flags(DroidMediaCamera *camera, int preview_callback_flag)
{
    if (preview_callback_flag & CAMERA_FRAME_CALLBACK_FLAG_ENABLE_MASK) {
        ALOGW("Preview frame callbacks are not supported yet");
    }
}

bool droid_media_camera_set_parameters(DroidMediaCamera *camera, const char *params)
{
    android::Mutex::Autolock locker(camera->m_lock);
    return camera->setParameters(params);
}

char *droid_media_camera_get_parameters(DroidMediaCamera *camera)
{
    android::Mutex::Autolock locker(camera->m_lock);
    return camera->flattenParameters();
}

bool droid_media_camera_take_picture(DroidMediaCamera *camera, int msgType)
{
    ALOGE("Still capture is not supported yet");
    return false;
}

void droid_media_camera_release_recording_frame(DroidMediaCamera *camera, DroidMediaCameraRecordingData *data)
{
    delete data;
}

nsecs_t droid_media_camera_recording_frame_get_timestamp(DroidMediaCameraRecordingData *data)
{
    return data->ts;
}

size_t droid_media_camera_recording_frame_get_size(DroidMediaCameraRecordingData *data)
{
    return 0;
}

void *droid_media_camera_recording_frame_get_data(DroidMediaCameraRecordingData *data)
{
    return NULL;
}

bool droid_media_camera_enable_face_detection(DroidMediaCamera *camera,
					      DroidMediaCameraFaceDetectionType type, bool enable)
{
    return !enable;
}

int32_t droid_media_camera_get_video_color_format (DroidMediaCamera *camera)
{
    return OMX_COLOR_FormatYUV420SemiPlanar;
}

};
