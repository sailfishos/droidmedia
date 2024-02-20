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
#    include "allocator.h"
#endif
#include "services/services.h"

using namespace android;

int main(int, char **)
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();

    MiniSurfaceFlinger::instantiate();

    // Android 4 will not allow system services to be run from minimediaservice.
    // So keep them here instead.

#if (ANDROID_MAJOR == 4)
    FakePermissionController::instantiate();
#    if (ANDROID_MINOR == 4)
    FakeAppOps::instantiate();
#    endif
#endif

    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
