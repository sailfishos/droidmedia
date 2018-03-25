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
#include "allocator.h"
#include "services/services.h"

using namespace android;

extern "C"
void startMiniSurfaceFlinger()
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();

    if (sm->checkService(String16("SurfaceFlinger")) == NULL)
    {
        MiniSurfaceFlinger::instantiate();
    }
    else
    {
        ALOGW("SurfaceFlinger service already running, so we won't start it here. If you have trouble with media, try disabling the minisf service.");
    }

    ProcessState::self()->startThreadPool();

}
