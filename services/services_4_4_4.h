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

using namespace android;

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

  bool authenticateSurfaceTexture(const sp<IGraphicBufferProducer>& surface) const {
    return true;
  }

#ifdef __arm__
  status_t captureScreen(const sp<IBinder>& display,
			 const sp<IGraphicBufferProducer>& producer,
			 uint32_t reqWidth, uint32_t reqHeight,
			 uint32_t minLayerZ, uint32_t maxLayerZ , bool) {
    return BAD_VALUE;
  }
#else
  bool isAnimationPermitted() {
    return false;
  }

  status_t captureScreen(const sp<IBinder>& display,
			 const sp<IGraphicBufferProducer>& producer,
			 uint32_t reqWidth, uint32_t reqHeight,
			 uint32_t minLayerZ, uint32_t maxLayerZ) {
    return BAD_VALUE;
  }
#endif
};

class FakePermissionController : public BinderService<FakePermissionController>,
                                 public BnPermissionController
{
public:
    static char const *getServiceName() {
        return "permission";
    }

    bool checkPermission(const String16& permission, int32_t, int32_t) {
        if (permission == String16("android.permission.CAMERA")) {
            return true;
        }

        return false;
    }

};

#include <binder/AppOpsManager.h>
#include <binder/IAppOpsService.h>
class FakeAppOps : public BinderService<FakeAppOps>,
           public BnAppOpsService
{
public:
  static char const *getServiceName() {
    return "appops";
  }

  virtual int32_t checkOperation(int32_t, int32_t, const String16&) {
    return android::AppOpsManager::MODE_ALLOWED;
  }

  virtual int32_t noteOperation(int32_t, int32_t, const String16&) {
    return android::AppOpsManager::MODE_ALLOWED;
  }

  virtual int32_t startOperation(const sp<IBinder>&, int32_t, int32_t,
                 const String16&) {
    return android::AppOpsManager::MODE_ALLOWED;
  }

  virtual void finishOperation(const sp<IBinder>&, int32_t, int32_t, const String16&) {
    // Nothing
  }

  virtual void startWatchingMode(int32_t, const String16&, const sp<IAppOpsCallback>&) {
    // Nothing
  }

  void stopWatchingMode(const sp<IAppOpsCallback>&) {
    // Nothing
  }

  virtual sp<IBinder> getToken(const sp<IBinder>&) {
    return NULL;
  }
};

