/*
 * Copyright (C) 2014-2015-2015 Jolla Ltd.
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

#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <CameraService.h>
#include <binder/MemoryHeapBase.h>
#if ANDROID_MAJOR <= 11
#include <MediaPlayerService.h>
#endif
#if ANDROID_MAJOR >= 6
#include <binder/BinderService.h>
#if ANDROID_MAJOR < 7
#include <camera/ICameraService.h>
#else
#include <android/hardware/ICameraService.h>
#endif
#include <binder/IInterface.h>
#include <cutils/multiuser.h>
#endif
#if ANDROID_MAJOR < 8
#include "allocator.h"
#endif
#include "services/services.h"
#if ANDROID_MAJOR >= 5
#include "services/audiopolicy/audiopolicy.h"
#endif

#include <cutils/properties.h>

#undef LOG_TAG
#define LOG_TAG "MinimediaService"

using namespace android;

#define BINDER_SERVICE_CHECK_INTERVAL 500000

int
main(int, char**)
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();

    // Disable things which break hybris once and for all.
    property_set("persist.camera.shutter.disable", "1");
    property_set("persist.media.metrics.enabled", "0");
    property_set("camera.fifo.disable", "1");

#if ANDROID_MAJOR >= 9
    FakeActivityManager::instantiate();
#endif
#if ANDROID_MAJOR <= 11
    MediaPlayerService::instantiate();
#endif
#if ANDROID_MAJOR >= 5
    FakeAudioPolicyService::instantiate();
#endif

// PermissionController and AppOps are needed on Android 4, but aren't allowed to be run here.
#if ANDROID_MAJOR >= 5
    FakePermissionController::instantiate();
    FakeAppOps::instantiate();
    FakeBatteryStats::instantiate();
#if ANDROID_MAJOR >= 10
    FakeSensorPrivacyManager::instantiate();
#endif
#if !defined(SENSORSERVER_DISABLE)
    FakeSensorServer::instantiate();
#endif
#endif
#if ANDROID_MAJOR >= 8
    sp<android::frameworks::sensorservice::V1_0::ISensorManager> sensorManager = new FakeSensorManager;
    status_t status = sensorManager->registerAsService();
    (void)status;
#endif
    CameraService::instantiate();
#if ANDROID_MAJOR >= 6
#if ANDROID_MAJOR <= 11
    FakeResourceManagerService::instantiate();
#endif
    FakeProcessInfoService::instantiate();
#if ANDROID_MAJOR < 8
    FakeCameraServiceProxy::instantiate();
#endif
    // Camera service needs to be told which users may use the camera
    sp<IBinder> binder;
    do {
        binder = sm->getService(String16("media.camera"));
        if (binder != NULL) {
            break;
        }
        ALOGW("Camera service is not yet available, waiting...");
        usleep(BINDER_SERVICE_CHECK_INTERVAL);
    } while (true);
    ALOGD("Allowing use of the camera for users root and bin");
#if ANDROID_MAJOR >= 7
    sp<hardware::ICameraService> gCameraService = interface_cast<hardware::ICameraService>(binder);
    std::vector<int32_t> users = {0, 1};
    gCameraService->notifySystemEvent(1, users);
#else
    sp<ICameraService> gCameraService = interface_cast<ICameraService>(binder);
    int32_t users[2];
    users[0] = 0; users[1] = 1;
    gCameraService->notifySystemEvent(1, users, 2);
#endif
#endif

    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
