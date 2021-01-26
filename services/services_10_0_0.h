/*
 * Copyright (C) 2014-2019 Jolla Ltd.
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

#include <gui/ISurfaceComposer.h>
#include <gui/IDisplayEventConnection.h>
#include <gui/ISurfaceComposerClient.h>
#include <ui/Rect.h>
#include <system/graphics.h>

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

    sp<ISurfaceComposerClient> createScopedConnection(
            const sp<IGraphicBufferProducer>&) {
        return sp<ISurfaceComposerClient>();
    }

    sp<IDisplayEventConnection> createDisplayEventConnection(
            VsyncSource, ConfigChanged) {
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

    sp<IBinder> getPhysicalDisplayToken(PhysicalDisplayId displayId) const{
        return NULL;
    }

    status_t getDisplayNativePrimaries(const sp<IBinder>& display, ui::DisplayPrimaries&) {
        return BAD_VALUE;
    }

    status_t getColorManagement(bool* outGetColorManagement) const{
        return BAD_VALUE;
    }

    void setTransactionState(const Vector<ComposerState>&, const Vector<DisplayState>&, uint32_t, const sp<IBinder>&,
                             const InputWindowCommands&, int64_t, const client_cache_t&, const std::vector<ListenerCallbacks>&) {
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

    status_t getDisplayViewport(const sp<IBinder>&, Rect*) {
        return 0;
    }

    int getActiveConfig(const sp<IBinder>&) {
        return 0;
    }

    status_t setActiveConfig(const sp<IBinder>&, int) {
        return BAD_VALUE;
    }

    status_t clearAnimationFrameStats() {
        return BAD_VALUE;
    }

    status_t getAnimationFrameStats(FrameStats*) const {
        return BAD_VALUE;
    }

    status_t getDisplayColorModes(const sp<IBinder>&,
            Vector<ui::ColorMode>*) {
        return BAD_VALUE;
    }

    ui::ColorMode getActiveColorMode(const sp<IBinder>&) {
        return static_cast<ui::ColorMode>(HAL_COLOR_MODE_NATIVE);
    }

    status_t setActiveColorMode(const sp<IBinder>&, ui::ColorMode) {
        return BAD_VALUE;
    }

    status_t getHdrCapabilities(const sp<IBinder>&, HdrCapabilities*) const {
        return BAD_VALUE;
    }

    status_t captureScreen(const sp<IBinder>&, sp<GraphicBuffer>*, bool&, const ui::Dataspace, const ui::PixelFormat, Rect,
                           uint32_t, uint32_t, bool, Rotation, bool) {
        return BAD_VALUE;
    }

    status_t captureScreen(const sp<IBinder>&, sp<GraphicBuffer>*, Rect, uint32_t, uint32_t, bool, Rotation) {
        return BAD_VALUE;
    }

    status_t captureScreen(uint64_t, ui::Dataspace*, sp<GraphicBuffer>*) {
        return BAD_VALUE;
    }

    status_t captureLayers(const sp<IBinder>&, sp<GraphicBuffer>*, const ui::Dataspace, const ui::PixelFormat,
                           const Rect&, const std::unordered_set<sp<IBinder>, SpHash<IBinder>>&, float, bool) {
        return 0;
    }

    status_t getSupportedFrameTimestamps(
            std::vector<FrameEvent>*) const {
        return BAD_VALUE;
    }

    status_t enableVSyncInjections(bool) {
        return BAD_VALUE;
    }

    status_t injectVSync(nsecs_t) {
        return BAD_VALUE;
    }

    status_t getLayerDebugInfo(std::vector<LayerDebugInfo>*) {
        return 0;
    }

    status_t getLayerDebugInfo(std::vector<LayerDebugInfo>*) const {
        return 0;
    }

    std::vector<PhysicalDisplayId> getPhysicalDisplayIds() const {
        return std::vector<PhysicalDisplayId>();
    }

    status_t getCompositionPreference(ui::Dataspace*, ui::PixelFormat*,ui::Dataspace*, ui::PixelFormat*) const {
        return BAD_VALUE;
    }

    status_t getDisplayedContentSamplingAttributes(const sp<IBinder>& display,  ui::PixelFormat*,
                                                   ui::Dataspace*, uint8_t*) const {
        return BAD_VALUE;
    }

    status_t setDisplayContentSamplingEnabled(const sp<IBinder>&, bool,uint8_t, uint64_t maxFrames) const {
        return BAD_VALUE;
    }

    status_t getDisplayedContentSample(const sp<IBinder>&, uint64_t, uint64_t,DisplayedFrameStats*) const {
        return BAD_VALUE;
    }

    status_t getProtectedContentSupport(bool* outSupported) const {
        return BAD_VALUE;
    }

    status_t isWideColorDisplay(const sp<IBinder>&, bool*) const {
        return BAD_VALUE;
    }

    status_t addRegionSamplingListener(const Rect&, const sp<IBinder>&, const sp<IRegionSamplingListener>&) {
        return BAD_VALUE;
    }

    status_t removeRegionSamplingListener(const sp<IRegionSamplingListener>&) {
        return BAD_VALUE;
    }

    status_t setAllowedDisplayConfigs(const sp<IBinder>&, const std::vector<int32_t>&) {
        return BAD_VALUE;
    }

    status_t getAllowedDisplayConfigs(const sp<IBinder>&, std::vector<int32_t>*) {
        return BAD_VALUE;
    }

    status_t getDisplayBrightnessSupport(const sp<IBinder>&, bool*) const {
        return BAD_VALUE;
    }

    status_t setDisplayBrightness(const sp<IBinder>&,float) const {
        return BAD_VALUE;
    }

    status_t notifyPowerHint(int32_t) {
        return BAD_VALUE;
    }
};

#include <binder/IPermissionController.h>

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

    int32_t noteOp(const String16&, int32_t, const String16&) {
        return 0;
    }

    void getPackagesForUid(const uid_t, Vector<String16> &) {
    }

    bool isRuntimePermission(const String16&) {
         return false;
    }

    int getPackageUid(const String16&, int) {
         return 0;
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
                 const String16&, bool) {
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

  virtual int32_t permissionToOpCode(const String16&) {
    return 0;
  }

  virtual int32_t checkAudioOperation(int32_t, int32_t, int32_t, const String16&) {
      return 0;
  }
};

#include <binder/IProcessInfoService.h>

class BnProcessInfoService : public BnInterface<IProcessInfoService> {
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0);
};

status_t BnProcessInfoService::onTransact( uint32_t code, const Parcel& data, Parcel* reply,
        uint32_t flags) {
    switch(code) {
        case GET_PROCESS_STATES_FROM_PIDS: {
            CHECK_INTERFACE(IProcessInfoService, data, reply);
            int32_t arrayLen = data.readInt32();
            if (arrayLen <= 0) {
                reply->writeNoException();
                reply->writeInt32(0);
                reply->writeInt32(NOT_ENOUGH_DATA);
                return NO_ERROR;
            }

            size_t len = static_cast<size_t>(arrayLen);
            int32_t pids[len];
            status_t res = data.read(pids, len * sizeof(*pids));

            // Ignore output array length returned in the parcel here, as the states array must
            // always be the same length as the input PIDs array.
            int32_t states[len];
            for (size_t i = 0; i < len; i++) states[i] = -1;
            if (res == NO_ERROR) {
                res = getProcessStatesFromPids(len, /*in*/ pids, /*out*/ states);
            }
            reply->writeNoException();
            reply->writeInt32Array(len, states);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        case GET_PROCESS_STATES_AND_OOM_SCORES_FROM_PIDS: {
            CHECK_INTERFACE(IProcessInfoService, data, reply);
            int32_t arrayLen = data.readInt32();
            if (arrayLen <= 0) {
                reply->writeNoException();
                reply->writeInt32(0);
                reply->writeInt32(NOT_ENOUGH_DATA);
                return NO_ERROR;
            }

            size_t len = static_cast<size_t>(arrayLen);
            int32_t pids[len];
            status_t res = data.read(pids, len * sizeof(*pids));

            // Ignore output array length returned in the parcel here, as the
            // states array must always be the same length as the input PIDs array.
            int32_t states[len];
            int32_t scores[len];
            for (size_t i = 0; i < len; i++) {
                states[i] = -1;
                scores[i] = -10000;
            }
            if (res == NO_ERROR) {
                res = getProcessStatesAndOomScoresFromPids(
                        len, /*in*/ pids, /*out*/ states, /*out*/ scores);
            }
            reply->writeNoException();
            reply->writeInt32Array(len, states);
            reply->writeInt32Array(len, scores);
            reply->writeInt32(res);
            return NO_ERROR;
        } break;
        default:
            return BBinder::onTransact(code, data, reply, flags);
    }
}

class FakeProcessInfoService : public BinderService<FakeProcessInfoService>,
                        public BnProcessInfoService
{
public:
    static char const *getServiceName() {
        return "processinfo";
    }

    status_t getProcessStatesFromPids(size_t length, int32_t* pids, int32_t* states) {
        for (unsigned int i=0; i< length; i++)
            states[i] = 0;
        return 0;
    }
    status_t getProcessStatesAndOomScoresFromPids( size_t length, int32_t* pids, int32_t* states, int32_t* scores) {
        for (unsigned int i=0; i< length; i++) {
            states[i] = 0;
            scores[i] = 0;
        }
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


#include <media/IResourceManagerService.h>
#include <media/IResourceManagerClient.h>
#include <media/MediaResource.h>
#include <media/MediaResourcePolicy.h>

class FakeResourceManagerService : public BinderService<FakeResourceManagerService>,
                         public BnResourceManagerService
{
public:
    static char const *getServiceName() {
        return "media.resource_manager";
    }
    void config(const Vector<MediaResourcePolicy> &) {
    }

    void addResource(int, int, int64_t, const sp<IResourceManagerClient>,
        const Vector<MediaResource> &) {
    }

    void removeResource(int, int64_t,
        const Vector<MediaResource> &) {
    }

    void removeClient(int, int64_t) {
    }

    bool reclaimResource(int, const Vector<MediaResource> &) {
        return true;
    }
};

#include <android/frameworks/sensorservice/1.0/IEventQueue.h>
#include <android/frameworks/sensorservice/1.0/ISensorManager.h>
#include <android/frameworks/sensorservice/1.0/types.h>
#include <android/hardware/sensors/1.0/types.h>

class FakeEventQueue :
    public android::frameworks::sensorservice::V1_0::IEventQueue
{
public:
    FakeEventQueue() {}

    android::hardware::Return<android::frameworks::sensorservice::V1_0::Result> enableSensor(
            int32_t sensorHandle, int32_t samplingPeriodUs, int64_t maxBatchReportLatencyUs) {
        return android::frameworks::sensorservice::V1_0::Result::BAD_VALUE;
    }

    android::hardware::Return<android::frameworks::sensorservice::V1_0::Result> disableSensor(
            int32_t sensorHandle) {
        return android::frameworks::sensorservice::V1_0::Result::BAD_VALUE;
    }
};

#include <binder/IActivityManager.h>

class BnFakeActivityManager : public BnInterface<IActivityManager>
{
public:
    virtual status_t    onTransact( uint32_t code,
                                    const Parcel& data,
                                    Parcel* reply,
                                    uint32_t flags = 0) {
        return NO_ERROR;
    };
};

class FakeActivityManager : public BinderService<FakeActivityManager>,
                                public BnFakeActivityManager
{
public:
    static char const *getServiceName() {
        return "activity";
    }
    
    virtual int openContentUri(const String16& stringUri) {
        return 0;
    };
    
    virtual void registerUidObserver(const sp<IUidObserver>& observer,
                                     const int32_t event,
                                     const int32_t cutpoint,
                                     const String16& callingPackage) {
    };
                                    
    virtual void unregisterUidObserver(const sp<IUidObserver>& observer) {
    };
    
    virtual bool isUidActive(const uid_t uid, const String16& callingPackage) {
        return false;
    };
    virtual int32_t getUidProcessState(const uid_t uid, const String16& callingPackage) {
        return 0;
    }
};

#include <android/hardware/BnSensorPrivacyManager.h>

class FakeSensorPrivacyManager : public BinderService<FakeSensorPrivacyManager>,
                                 public hardware::BnSensorPrivacyManager
{
public:
    static char const *getServiceName() {
        return "sensor_privacy";
    }

    ::android::binder::Status addSensorPrivacyListener(const sp<hardware::ISensorPrivacyListener>& listener) {
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status removeSensorPrivacyListener(const sp<hardware::ISensorPrivacyListener>& listener) {
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status isSensorPrivacyEnabled(bool *enabled) {
        *enabled = false;
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status setSensorPrivacy(bool enable) {
        return ::android::binder::Status::ok();
    }
};
