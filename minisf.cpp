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
#include <binder/BinderService.h>
#include <gui/ISurfaceComposer.h>
#include <gui/IDisplayEventConnection.h>
#include <binder/IPermissionController.h>
#include <binder/MemoryHeapBase.h>
#include "allocator.h"

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 1 && ANDROID_MICRO == 2
#include "services/services_4_1_2.h"
#endif

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 2 && ANDROID_MICRO == 2
#include "services/services_4_2_2.h"
#endif

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4 && ANDROID_MICRO == 4
#include "services/services_4_4_4.h"
#endif

#if ANDROID_MAJOR == 5 && ANDROID_MINOR == 1
#include "services/services_5_1_0.h"
#endif

#if ANDROID_MAJOR == 6 && ANDROID_MINOR == 0
#include "services/services_6_0_0.h"
#endif

#if ANDROID_MAJOR == 7 && ANDROID_MINOR == 1
#include "services/services_7_1_0.h"
#endif

using namespace android;

int
main(int, char**)
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();

    FakePermissionController::instantiate();
    MiniSurfaceFlinger::instantiate();

#if (ANDROID_MAJOR == 4 && ANDROID_MINOR == 4) || ANDROID_MAJOR >= 5
    FakeAppOps::instantiate();
#endif
   
#if ANDROID_MAJOR >= 5
    FakeBatteryStats::instantiate();
    FakeSensorServer::instantiate();
#endif

#if ANDROID_MAJOR >= 6
    FakeProcessInfoService::instantiate();
#endif

    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
