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

#include <ui/Rect.h>

using namespace android;

class MiniSurfaceFlinger : public BinderService<MiniSurfaceFlinger>,
                           public BnSurfaceComposer,
                           public IBinder::DeathRecipient
{
public:
    static char const *getServiceName() {
        return "SurfaceFlinger";
    }

    void binderDied(const wp<IBinder>&) {
        // Nothing
    }

    sp<ISurfaceComposerClient> createConnection() {
        return sp<ISurfaceComposerClient>();
    }

    sp<IGraphicBufferAlloc> createGraphicBufferAlloc() {
        sp<DroidMediaAllocator> gba(new DroidMediaAllocator());
        return gba;
    }

    sp<IDisplayEventConnection> createDisplayEventConnection() {
        return sp<IDisplayEventConnection>();
    }

    sp<IBinder> createDisplay(const String8&, bool) {
        return NULL;
    }

    void destroyDisplay(const sp<IBinder>&) {
        // Nothing
    }

    virtual sp<IBinder> getBuiltInDisplay(int32_t) {
        return NULL;
    }

    void setTransactionState(const Vector<ComposerState>&, const Vector<DisplayState>&, uint32_t) {
        // Nothing
    }

    void bootFinished() {
      // Nothing
    }

    bool authenticateSurfaceTexture(const sp<IGraphicBufferProducer>&) const {
        return true;
    }

    void setPowerMode(const sp<IBinder>&, int) {

    }

    status_t getDisplayConfigs(const sp<IBinder>&, Vector<DisplayInfo>*) {
        return BAD_VALUE;
    }

    status_t getDisplayStats(const sp<IBinder>&, DisplayStatInfo*) {
        return BAD_VALUE;
    }

    int getActiveConfig(const sp<IBinder>&) {
        return 0;
    }

    status_t setActiveConfig(const sp<IBinder>&, int) {
        return BAD_VALUE;
    }

    status_t captureScreen(const sp<IBinder>&, const sp<IGraphicBufferProducer>&,
			   Rect, uint32_t, uint32_t, uint32_t, uint32_t,
			   bool, Rotation
#ifdef CM_BUILD
			   , bool
#endif
			   ) {
        return BAD_VALUE;
    }

    status_t clearAnimationFrameStats() {
        return BAD_VALUE;
    }

    status_t getAnimationFrameStats(FrameStats*) const {
        return BAD_VALUE;
    }
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

    void getPackagesForUid(const uid_t uid, Vector<String16> &packages) {
    }

    bool isRuntimePermission(const String16& permission) {
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
  
  virtual int32_t permissionToOpCode(const String16& permission) {
    return 0;
  }  
};

#include <binder/IProcessInfoService.h>

class FakeProcessInfoService : public BinderService<FakeProcessInfoService>,
                        public BnProcessInfoService
{
public:
    static char const *getServiceName() {
        return "processinfo";
    }

    status_t getProcessStatesFromPids(size_t length, int32_t* pids, int32_t* states) {
        for (int i=0; i< length; i++)
            states[i] = 0;
        return 0;
    }
};

#include <binder/IBatteryStats.h>

class FakeBatteryStats : public BinderService<FakeBatteryStats>,
                                public BnBatteryStats
{
public:
    static char const *getServiceName() {
        return "batterystats";
    }
    void noteStartSensor(int uid, int sensor) {  }
    void noteStopSensor(int uid, int sensor) {  }
    void noteStartVideo(int uid) {  }
    void noteStopVideo(int uid) {  }
    void noteStartAudio(int uid) {  }
    void noteStopAudio(int uid) {  }
    void noteResetVideo() {  }
    void noteResetAudio() {  }
    void noteFlashlightOn(int uid) {  }
    void noteFlashlightOff(int uid) {  }
    void noteStartCamera(int uid) {  }
    void noteStopCamera(int uid) {  }
    void noteResetCamera() {  }
    void noteResetFlashlight() {  }
};


#include <gui/ISensorServer.h>
#include <gui/ISensorEventConnection.h>
#include <gui/Sensor.h>
#include <gui/BitTube.h>

class FakeSensorEventConnection : public BnSensorEventConnection
{
    sp<BitTube> mChannel;
public:
    FakeSensorEventConnection()
    {
        mChannel = new BitTube(0);
    }
    sp<BitTube> getSensorChannel() const {
        return mChannel;
    }
    status_t enableDisable(int handle, bool enabled, nsecs_t samplingPeriodNs,
                            nsecs_t maxBatchReportLatencyNs, int reservedFlags) {
        return 0;
    }
    status_t setEventRate(int handle, nsecs_t ns) {
        return 0;
    }
    status_t flush() {
        return 0;
    }
};
class FakeSensorServer : public BinderService<FakeSensorServer>,
                         public BnSensorServer
{
public:
    static char const *getServiceName() {
        return "sensorservice";
    }

    Vector<Sensor> getSensorList(const String16& opPackageName) {
        return Vector<Sensor>();
    }

    sp<ISensorEventConnection> createSensorEventConnection(const String8& packageName,
                        int mode, const String16& opPackageName) {
        return sp<ISensorEventConnection>(new FakeSensorEventConnection);
    }

    int32_t isDataInjectionEnabled() {
        return 0;
    }
};
