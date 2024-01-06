/*
 * Copyright (C) 2014-2021 Jolla Ltd.
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

#include <sensor/ISensorServer.h>
#include <sensor/ISensorEventConnection.h>
#include <sensor/Sensor.h>
#include <sensor/BitTube.h>

class FakeSensorEventConnection : public android::BnSensorEventConnection
{
    android::sp<android::BitTube> mChannel;
public:
    FakeSensorEventConnection() {
        mChannel = new android::BitTube(0);
    }

    android::sp<android::BitTube> getSensorChannel() const {
        return mChannel;
    }

    android::status_t enableDisable(int, bool, nsecs_t, nsecs_t, int) {
        return 0;
    }

    android::status_t setEventRate(int, nsecs_t) {
        return 0;
    }

    android::status_t flush() {
        return 0;
    }

    virtual int32_t configureChannel(int32_t, int32_t) {
        return 0;
    }

protected:
    void destroy() {
    }

};

class FakeSensorServer : public android::BinderService<FakeSensorServer>,
                         public android::BnSensorServer
{
public:
    static char const *getServiceName() {
        return "sensorservice";
    }

    android::Vector<android::Sensor> getSensorList(const android::String16&) {
        return android::Vector<android::Sensor>();
    }

    android::Vector<android::Sensor> getDynamicSensorList(const android::String16&) {
        return android::Vector<android::Sensor>();
    }

    android::sp<android::ISensorEventConnection> createSensorEventConnection(
            const android::String8&, int, const android::String16&,
            const android::String16&) {
        return android::sp<android::ISensorEventConnection>(new FakeSensorEventConnection);
    }

    android::sp<android::ISensorEventConnection> createSensorDirectConnection(
            const android::String16&, uint32_t, int32_t, int32_t, const native_handle_t *) {
        return android::sp<android::ISensorEventConnection>(new FakeSensorEventConnection);
    }

    int setOperationParameter(
            int32_t, int32_t, const android::Vector<float> &, const android::Vector<int32_t> &) {
        return 0;
    }

    int32_t isDataInjectionEnabled() {
        return 0;
    }

    android::status_t shellCommand(int, int, int,
            android::Vector<android::String16>&) {
        return 0;
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

class FakeSensorManager :
    public android::frameworks::sensorservice::V1_0::ISensorManager
{

    // Methods from ::android::frameworks::sensorservice::V1_0::ISensorManager follow.
    android::hardware::Return<void> getSensorList(getSensorList_cb _hidl_cb) {
        android::hardware::hidl_vec<::android::hardware::sensors::V1_0::SensorInfo> ret;
        _hidl_cb(ret, android::frameworks::sensorservice::V1_0::Result::OK);
        return android::hardware::Void();
    }

    android::hardware::Return<void> getDefaultSensor(
            android::hardware::sensors::V1_0::SensorType type,
            getDefaultSensor_cb _hidl_cb) {
        _hidl_cb({}, android::frameworks::sensorservice::V1_0::Result::NOT_EXIST);
        return android::hardware::Void();
    }

    android::hardware::Return<void> createAshmemDirectChannel(
            const android::hardware::hidl_memory& mem, uint64_t size,
            createAshmemDirectChannel_cb _hidl_cb) {
        _hidl_cb(nullptr, android::frameworks::sensorservice::V1_0::Result::BAD_VALUE);
        return android::hardware::Void();
    }

    android::hardware::Return<void> createGrallocDirectChannel(
            const android::hardware::hidl_handle& buffer, uint64_t size,
            createGrallocDirectChannel_cb _hidl_cb) {
        _hidl_cb(nullptr, android::frameworks::sensorservice::V1_0::Result::UNKNOWN_ERROR);
        return android::hardware::Void();
    }

    android::hardware::Return<void> createEventQueue(
            const android::sp<android::frameworks::sensorservice::V1_0::IEventQueueCallback> &callback,
            createEventQueue_cb _hidl_cb) {
        if (callback == nullptr) {
            _hidl_cb(nullptr, android::frameworks::sensorservice::V1_0::Result::BAD_VALUE);
            return android::hardware::Void();
        }

        android::sp<android::frameworks::sensorservice::V1_0::IEventQueue> queue = new FakeEventQueue();

        _hidl_cb(queue, android::frameworks::sensorservice::V1_0::Result::OK);
        return android::hardware::Void();
    }
};

using namespace android;

#include <gui/ISurfaceComposer.h>
#include <android/gui/IDisplayEventConnection.h>
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

    sp<IDisplayEventConnection> createDisplayEventConnection(
            VsyncSource, EventRegistrationFlags) {
        return sp<IDisplayEventConnection>();
    }

    status_t setTransactionState(const FrameTimelineInfo&, const Vector<ComposerState>&,
                             const Vector<DisplayState>&, uint32_t, const sp<IBinder>&,
                             const InputWindowCommands&, int64_t, bool,  const client_cache_t&,
                             bool, const std::vector<ListenerCallbacks>&, uint64_t) {
        return BAD_VALUE;
    }

    void bootFinished() {
        // Nothing
    }

    bool authenticateSurfaceTexture(const sp<IGraphicBufferProducer>&) const {
        return true;
    }

    status_t getSupportedFrameTimestamps(
            std::vector<FrameEvent>*) const {
        return BAD_VALUE;
    }

    status_t getStaticDisplayInfo(const sp<IBinder>&, ui::StaticDisplayInfo*) {
        return BAD_VALUE;
    }

    status_t getDynamicDisplayInfo(const sp<IBinder>&, ui::DynamicDisplayInfo*) {
        return BAD_VALUE;
    }

    status_t getDisplayNativePrimaries(const sp<IBinder>& display, ui::DisplayPrimaries&) {
        return BAD_VALUE;
    }

    status_t setActiveColorMode(const sp<IBinder>&, ui::ColorMode) {
        return BAD_VALUE;
    }

    status_t setBootDisplayMode(const sp<IBinder>&, ui::DisplayModeId) {
        return BAD_VALUE;
    }

    status_t clearAnimationFrameStats() {
        return BAD_VALUE;
    }

    status_t getAnimationFrameStats(FrameStats*) const {
        return BAD_VALUE;
    }

    status_t overrideHdrTypes(const sp<IBinder>&, const std::vector<ui::Hdr>&) {
        return BAD_VALUE;
    }

    status_t onPullAtom(const int32_t, std::string*, bool*) {
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

    status_t getColorManagement(bool* outGetColorManagement) const {
        return BAD_VALUE;
    }

    status_t getCompositionPreference(ui::Dataspace*, ui::PixelFormat*,ui::Dataspace*, ui::PixelFormat*) const {
        return BAD_VALUE;
    }

    status_t getDisplayedContentSamplingAttributes(const sp<IBinder>& display, ui::PixelFormat*,
                                                   ui::Dataspace*, uint8_t*) const {
        return BAD_VALUE;
    }

    status_t setDisplayContentSamplingEnabled(const sp<IBinder>&, bool, uint8_t, uint64_t) {
        return BAD_VALUE;
    }

    status_t getDisplayedContentSample(const sp<IBinder>&, uint64_t, uint64_t, DisplayedFrameStats*) const {
        return BAD_VALUE;
    }

    status_t getProtectedContentSupport(bool* outSupported) const {
        return BAD_VALUE;
    }

    status_t addRegionSamplingListener(const Rect&, const sp<IBinder>&, const sp<IRegionSamplingListener>&) {
        return BAD_VALUE;
    }

    status_t removeRegionSamplingListener(const sp<IRegionSamplingListener>&) {
        return BAD_VALUE;
    }

    status_t addFpsListener(int32_t, const sp<gui::IFpsListener>&) {
        return BAD_VALUE;
    }

    status_t removeFpsListener(const sp<gui::IFpsListener>&) {
        return BAD_VALUE;
    }

    status_t addTunnelModeEnabledListener(const sp<gui::ITunnelModeEnabledListener>&) {
        return BAD_VALUE;
    }

    status_t removeTunnelModeEnabledListener(const sp<gui::ITunnelModeEnabledListener>&) {
        return BAD_VALUE;
    }

    status_t setDesiredDisplayModeSpecs(const sp<IBinder>&, ui::DisplayModeId,
                                        bool, float, float, float, float) {
        return BAD_VALUE;
    }

    status_t getDesiredDisplayModeSpecs(const sp<IBinder>&, ui::DisplayModeId*,
                                        bool*, float*, float*, float*, float*) {
        return BAD_VALUE;
    }

    status_t setGlobalShadowSettings(const half4&, const half4&, float, float, float) {
        return BAD_VALUE;
    }

    status_t getDisplayDecorationSupport(const sp<IBinder>&,
            std::optional<aidl::android::hardware::graphics::common::DisplayDecorationSupport>*) const {
        return BAD_VALUE;
    }

    status_t setFrameRate(const sp<IGraphicBufferProducer>&, float, int8_t, int8_t) {
        return BAD_VALUE;
    }

    status_t setOverrideFrameRate(uid_t, float) {
        return BAD_VALUE;
    }

    status_t setFrameTimelineInfo(const sp<IGraphicBufferProducer>&,
                                  const FrameTimelineInfo&) {
        return BAD_VALUE;
    }

    status_t addTransactionTraceListener(const sp<gui::ITransactionTraceListener>&) {
        return BAD_VALUE;
    }

    int getGPUContextPriority() {
        return 0;
    }

    status_t getMaxAcquiredBufferCount(int*) const {
        return BAD_VALUE;
    }

    status_t addWindowInfosListener(const sp<gui::IWindowInfosListener>&) const {
        return BAD_VALUE;
    }

    status_t removeWindowInfosListener(const sp<gui::IWindowInfosListener>&) const {
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

    int32_t checkOperation(int32_t, int32_t, const String16&) {
        return android::AppOpsManager::MODE_ALLOWED;
    }

    int32_t noteOperation(int32_t, int32_t, const String16&,
                          const std::optional<String16>&, bool,
                          const String16&, bool) {
        return android::AppOpsManager::MODE_ALLOWED;
    }

    int32_t startOperation(const sp<IBinder>&, int32_t, int32_t,
                           const String16&, const std::optional<String16>&,
                           bool, bool, const String16&, bool) {
        return android::AppOpsManager::MODE_ALLOWED;
    }

    void finishOperation(const sp<IBinder>&, int32_t, int32_t,
                         const String16&, const std::optional<String16>&) {
        // Nothing
    }

    void startWatchingMode(int32_t, const String16&, const sp<IAppOpsCallback>&) {
        // Nothing
    }

    void stopWatchingMode(const sp<IAppOpsCallback>&) {
        // Nothing
    }

    sp<IBinder> getToken(const sp<IBinder>&) {
        return NULL;
    }

    int32_t permissionToOpCode(const String16&) {
        return 0;
    }

    int32_t checkAudioOperation(int32_t, int32_t, int32_t, const String16&) {
        return 0;
    }

    void setCameraAudioRestriction(int32_t) {
        // Nothing
    }

    bool shouldCollectNotes(int32_t) {
        return false;
    }

    void startWatchingModeWithFlags(int32_t, const String16&, int32_t, const sp<IAppOpsCallback>&) {
        // Nothing
    }
};

#include <processinfo/IProcessInfoService.h>

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

#include <batterystats/IBatteryStats.h>

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

#include <aidl/android/media/IResourceManagerService.h>
#include <aidl/android/media/IResourceManagerClient.h>
#include <aidl/android/media/BnResourceManagerService.h>
#include <media/MediaResource.h>
#include <media/MediaResourcePolicy.h>

#include <android/binder_manager.h>
//#include <android/binder_process.h>

class FakeResourceManagerService : public BinderService<FakeResourceManagerService>,
                         public aidl::android::media::BnResourceManagerService
{
public:
    static char const *getServiceName() {
        return "media.resource_manager";
    }

    static void instantiate() {
        std::shared_ptr<FakeResourceManagerService> service =
                ::ndk::SharedRefBase::make<FakeResourceManagerService>();
        binder_status_t status =
                AServiceManager_addService(service->asBinder().get(), getServiceName());
        if (status != STATUS_OK) {
            return;
        }
        // TODO: mediaserver main() is already starting the thread pool,
        // move this to mediaserver main() when other services in mediaserver
        // are converted to ndk-platform aidl.
        //ABinderProcess_startThreadPool();
    }

    ::ndk::ScopedAStatus config(const std::vector<::aidl::android::media::MediaResourcePolicyParcel>&) {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus addResource(int32_t, int32_t, int64_t,
                                     const std::shared_ptr<::aidl::android::media::IResourceManagerClient>&,
                                     const std::vector<::aidl::android::media::MediaResourceParcel>&) {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus removeResource(int32_t, int64_t,
                                        const std::vector<::aidl::android::media::MediaResourceParcel>&) {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus removeClient(int32_t, int64_t) {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus reclaimResource(int32_t,
                                         const std::vector<::aidl::android::media::MediaResourceParcel>&,
                                         bool*) {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus overridePid(int32_t, int32_t) {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus overrideProcessInfo(const std::shared_ptr<::aidl::android::media::IResourceManagerClient>&,
                                             int32_t, int32_t, int32_t) {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus markClientForPendingRemoval(int32_t, int64_t) {
        return ::ndk::ScopedAStatus::ok();
    }

    ::ndk::ScopedAStatus reclaimResourcesFromClientsPendingRemoval(int32_t) {
        return ::ndk::ScopedAStatus::ok();
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

    virtual int openContentUri(const String16&) {
        return 0;
    };

    virtual status_t registerUidObserver(const sp<IUidObserver>&,
                                         const int32_t,
                                         const int32_t,
                                         const String16&) {
        return BAD_VALUE;
    };

    virtual status_t unregisterUidObserver(const sp<IUidObserver>&) {
        return BAD_VALUE;
    };

    virtual bool isUidActive(const uid_t, const String16&) {
        return false;
    };

    virtual int32_t getUidProcessState(const uid_t, const String16&) {
        return 0;
    }

    virtual status_t checkPermission(const String16&,
                                     const pid_t,
                                     const uid_t,
                                     int32_t*) {
        return BAD_VALUE;
    };
};

#include <android/hardware/BnSensorPrivacyManager.h>

class FakeSensorPrivacyManager : public BinderService<FakeSensorPrivacyManager>,
                                 public hardware::BnSensorPrivacyManager
{
public:
    static char const *getServiceName() {
        return "sensor_privacy";
    }

    ::android::binder::Status supportsSensorToggle(int32_t, int32_t, bool *supported) override {
        *supported = false;
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status addSensorPrivacyListener(const sp<hardware::ISensorPrivacyListener>&) override {
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status removeSensorPrivacyListener(const sp<hardware::ISensorPrivacyListener>&) override {
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status isSensorPrivacyEnabled(bool *enabled) override {
        *enabled = false;
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status setSensorPrivacy(bool) override {
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status addToggleSensorPrivacyListener(const ::android::sp<::android::hardware::ISensorPrivacyListener>&) override {
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status removeToggleSensorPrivacyListener(const ::android::sp<::android::hardware::ISensorPrivacyListener>&) override {
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status isCombinedToggleSensorPrivacyEnabled(int32_t, bool *enabled) override {
        *enabled = false;
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status isToggleSensorPrivacyEnabled(int32_t, int32_t, bool *enabled) override {
        *enabled = false;
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status setToggleSensorPrivacy(int32_t, int32_t, int32_t, bool) override {
        return ::android::binder::Status::ok();
    }

    ::android::binder::Status setToggleSensorPrivacyForProfileGroup(int32_t, int32_t, int32_t, bool) override {
        return ::android::binder::Status::ok();
    }

};
