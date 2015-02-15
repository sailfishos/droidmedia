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

using namespace android;

class FakePermissionController : public BinderService<FakePermissionController>,
                                 public BnPermissionController
{
public:
    static char const *getServiceName() {
        return "permission";
    }

    bool checkPermission(const String16& permission,
                         int32_t pid, int32_t uid) {
        return true; // DUH! :P
    }
};

class MiniSurfaceFlinger : public BinderService<MiniSurfaceFlinger>,
                           public BnSurfaceComposer,
                           public IBinder::DeathRecipient
{
public:
    static char const *getServiceName() {
        return "SurfaceFlinger";
    }

    void binderDied(const wp<IBinder>& who) {
        // Nothing
    }

    sp<ISurfaceComposerClient> createConnection() {
        return sp<ISurfaceComposerClient>();
    }

    sp<IGraphicBufferAlloc> createGraphicBufferAlloc() {
        sp<DroidMediaAllocator> gba(new DroidMediaAllocator());
        return gba;
    }

    void bootFinished() {
        // Nothing
    }

    sp<IDisplayEventConnection> createDisplayEventConnection() {
        return sp<IDisplayEventConnection>();
    }

#if ANDROID_MAJOR == 4 && (ANDROID_MINOR == 4 || ANDROID_MINOR == 2)
  sp<IBinder> createDisplay(const String8& displayName, bool secure) {
    return NULL;
  }

  void destroyDisplay(const sp<IBinder>& display) {
    // Nothing
  }

  void setTransactionState(const Vector<ComposerState>& state,
			   const Vector<DisplayState>& displays, uint32_t flags) {
    // Nothing
  }

  void blank(const sp<IBinder>& display) {
    // Nothing
  }

  void unblank(const sp<IBinder>& display) {
    // Nothing
  }

  virtual sp<IBinder> getBuiltInDisplay(int32_t id) {
    return NULL;
  }

  status_t getDisplayInfo(const sp<IBinder>& display, DisplayInfo* info) {
    return BAD_VALUE;
  }

#if ANDROID_MINOR == 4
  bool authenticateSurfaceTexture(const sp<IGraphicBufferProducer>& surface) const {
    return true;
  }

  status_t captureScreen(const sp<IBinder>& display,
			 const sp<IGraphicBufferProducer>& producer,
			 uint32_t reqWidth, uint32_t reqHeight,
			 uint32_t minLayerZ, uint32_t maxLayerZ) {
    return BAD_VALUE;
  }
#endif

#if ANDROID_MINOR == 2
  bool authenticateSurfaceTexture(const sp<ISurfaceTexture>& surface) const {
    return true;
  }

  status_t captureScreen(const sp<IBinder>& display, sp<IMemoryHeap>* heap,
                           uint32_t* width, uint32_t* height, PixelFormat* format,
                           uint32_t reqWidth, uint32_t reqHeight,
                           uint32_t minLayerZ, uint32_t maxLayerZ) {
        return BAD_VALUE;
  }
#endif

  bool isAnimationPermitted() {
    return false;
  }

#else
    sp<IMemoryHeap> getCblk() const {
        static android::sp<android::MemoryHeapBase>
            mem(new MemoryHeapBase(4096,
                                   MemoryHeapBase::READ_ONLY, "SurfaceFlinger read-only heap"));
        return mem;
    }

    void setTransactionState(const Vector<ComposerState>& state,
                             int orientation, uint32_t flags) {
        // Nothing
    }

    status_t captureScreen(DisplayID dpy, sp<IMemoryHeap> *heap,
                           uint32_t* width, uint32_t* height, PixelFormat* format,
                           uint32_t reqWidth, uint32_t reqHeight,
                           uint32_t minLayerZ, uint32_t maxLayerZ) {
        return BAD_VALUE;
    }

    virtual status_t turnElectronBeamOff(int32_t mode) {
        return BAD_VALUE;
    }

    virtual status_t turnElectronBeamOn(int32_t mode) {
        return BAD_VALUE;
    }

    bool authenticateSurfaceTexture(const sp<ISurfaceTexture>& surface) const {
        return false;
    }
#endif
};

int
main(int argc, char* argv[])
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();

    FakePermissionController::instantiate();
    MiniSurfaceFlinger::instantiate();

    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
