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
#include <binder/MemoryHeapBase.h>
#if ANDROID_MAJOR < 8
#include "allocator.h"
#endif
#include "services/services.h"
#ifdef USE_MOTO_SF
#include "SurfaceFlinger.h"
#endif

using namespace android;

int
main(int, char**)
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();

#ifdef USE_MOTO_SF
    ProcessState::self()->setThreadPoolMaxThreadCount(4);
    ALOGI("MOTO_SF SurfaceFlinger: start the thread pool"); 
    // start the thread pool
    sp<ProcessState> ps(ProcessState::self());
    ps->startThreadPool();

    ALOGI("MOTO_SF SurfaceFlinger: create SurfaceFlinger"); 
    sp<SurfaceFlinger> flinger = new SurfaceFlinger();
    // initialize before clients can connect
    ALOGI("MOTO_SF SurfaceFlinger: call SurfaceFlinger->init()"); 
    flinger->init();
    // publish surface flinger
    ALOGI("MOTO_SF SurfaceFlinger: publish SurfaceFlinger"); 
    sm->addService(String16(SurfaceFlinger::getServiceName()), flinger, false);
    ALOGI("MOTO_SF SurfaceFlinger: call SurfaceFlinger->run()"); 
    flinger->run();
    ALOGI("MOTO_SF SurfaceFlinger: after SurfaceFlinger->run()"); 
#else
     MiniSurfaceFlinger::instantiate();
#endif

// Android 4 will not allow system services to be run from minimediaservice. So keep them here instead.

#if (ANDROID_MAJOR == 4)
    FakePermissionController::instantiate();
#if (ANDROID_MINOR == 4)
    FakeAppOps::instantiate();
#endif
#endif


    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
