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

#include <gui/ISurfaceComposer.h>
#include <gui/IDisplayEventConnection.h>
#include <ui/Rect.h>
#include <system/graphics.h>

class MiniSurfaceFlinger : public BinderService<MiniSurfaceFlinger>,
                           public BnSurfaceComposer,
                           public IBinder::DeathRecipient {
public:
    static char const *getServiceName()
    {
        return "SurfaceFlinger";
    }

    void binderDied(const wp<IBinder> &)
    {
        // Nothing
    }

    sp<ISurfaceComposerClient> createConnection()
    {
        return sp<ISurfaceComposerClient>();
    }

    sp<IGraphicBufferAlloc> createGraphicBufferAlloc()
    {
        sp<DroidMediaAllocator> gba(new DroidMediaAllocator());
        return gba;
    }

    sp<IDisplayEventConnection> createDisplayEventConnection()
    {
        return sp<IDisplayEventConnection>();
    }

    sp<IBinder> createDisplay(const String8 &, bool)
    {
        return NULL;
    }

    void destroyDisplay(const sp<IBinder> &)
    {
        // Nothing
    }

    virtual sp<IBinder> getBuiltInDisplay(int32_t)
    {
        return NULL;
    }

    void setTransactionState(const Vector<ComposerState> &, const Vector<DisplayState> &, uint32_t)
    {
        // Nothing
    }

    void bootFinished()
    {
        // Nothing
    }

    bool authenticateSurfaceTexture(const sp<IGraphicBufferProducer> &) const
    {
        return true;
    }

    void setPowerMode(const sp<IBinder> &, int)
    {
    }

    status_t getDisplayConfigs(const sp<IBinder> &, Vector<DisplayInfo> *)
    {
        return BAD_VALUE;
    }

    status_t getDisplayStats(const sp<IBinder> &, DisplayStatInfo *)
    {
        return BAD_VALUE;
    }

    int getActiveConfig(const sp<IBinder> &)
    {
        return 0;
    }

    status_t setActiveConfig(const sp<IBinder> &, int)
    {
        return BAD_VALUE;
    }

    status_t captureScreen(const sp<IBinder> &, const sp<IGraphicBufferProducer> &, Rect, uint32_t,
        uint32_t, uint32_t, uint32_t, bool, Rotation
#ifdef CM_BUILD
        ,
        bool
#endif
    )
    {
        return BAD_VALUE;
    }

    status_t clearAnimationFrameStats()
    {
        return BAD_VALUE;
    }

    status_t getAnimationFrameStats(FrameStats *) const
    {
        return BAD_VALUE;
    }

    status_t getDisplayColorModes(const sp<IBinder> &display, Vector<android_color_mode_t> *configs)
    {
        return BAD_VALUE;
    }

    android_color_mode_t getActiveColorMode(const sp<IBinder> &display)
    {
        return HAL_COLOR_MODE_NATIVE;
    }

    status_t setActiveColorMode(const sp<IBinder> &display, android_color_mode_t colorMode)
    {
        return BAD_VALUE;
    }

    status_t getHdrCapabilities(const sp<IBinder> &display, HdrCapabilities *outCapabilities) const
    {
        return BAD_VALUE;
    }
};

#include <binder/IPermissionController.h>

class FakePermissionController : public BinderService<FakePermissionController>,
                                 public BnPermissionController {
public:
    static char const *getServiceName()
    {
        return "permission";
    }

    bool checkPermission(const String16 &permission, int32_t, int32_t)
    {
        if (permission == String16("android.permission.CAMERA")) {
            return true;
        }

        return false;
    }

    void getPackagesForUid(const uid_t uid, Vector<String16> &packages)
    {
    }

    bool isRuntimePermission(const String16 &permission)
    {
        return false;
    }
};

#include <binder/AppOpsManager.h>
#include <binder/IAppOpsService.h>
class FakeAppOps : public BinderService<FakeAppOps>, public BnAppOpsService {
public:
    static char const *getServiceName()
    {
        return "appops";
    }

    virtual int32_t checkOperation(int32_t, int32_t, const String16 &)
    {
        return android::AppOpsManager::MODE_ALLOWED;
    }

    virtual int32_t noteOperation(int32_t, int32_t, const String16 &)
    {
        return android::AppOpsManager::MODE_ALLOWED;
    }

    virtual int32_t startOperation(const sp<IBinder> &, int32_t, int32_t, const String16 &)
    {
        return android::AppOpsManager::MODE_ALLOWED;
    }

    virtual void finishOperation(const sp<IBinder> &, int32_t, int32_t, const String16 &)
    {
        // Nothing
    }

    virtual void startWatchingMode(int32_t, const String16 &, const sp<IAppOpsCallback> &)
    {
        // Nothing
    }

    void stopWatchingMode(const sp<IAppOpsCallback> &)
    {
        // Nothing
    }

    virtual sp<IBinder> getToken(const sp<IBinder> &)
    {
        return NULL;
    }

    virtual int32_t permissionToOpCode(const String16 &permission)
    {
        return 0;
    }
};

#include <binder/IProcessInfoService.h>

class BnProcessInfoService : public BnInterface<IProcessInfoService> {
public:
    virtual status_t onTransact(
        uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags = 0);
};

status_t BnProcessInfoService::onTransact(
    uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags)
{
    switch (code) {
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

        // Ignore output array length returned in the parcel here, as the states
        // array must always be the same length as the input PIDs array.
        int32_t states[len];
        for (size_t i = 0; i < len; i++)
            states[i] = -1;
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
            res = getProcessStatesAndOomScoresFromPids(len, /*in*/ pids, /*out*/ states,
                /*out*/ scores);
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
                               public BnProcessInfoService {
public:
    static char const *getServiceName()
    {
        return "processinfo";
    }

    status_t getProcessStatesFromPids(size_t length, int32_t *pids, int32_t *states)
    {
        for (int i = 0; i < length; i++)
            states[i] = 0;
        return 0;
    }
    status_t getProcessStatesAndOomScoresFromPids(
        size_t length, int32_t *pids, int32_t *states, int32_t *scores)
    {
        for (int i = 0; i < length; i++) {
            states[i] = 0;
            scores[i] = 0;
        }
        return 0;
    }
};

#include <camera/ICameraServiceProxy.h>

class FakeCameraServiceProxy : public BinderService<FakeCameraServiceProxy>,
                               public BnCameraServiceProxy {
public:
    static char const *getServiceName()
    {
        return "media.camera.proxy";
    }

    void pingForUserUpdate()
    {
    }

    void notifyCameraState(String16 cameraId, CameraState newCameraState)
    {
    }
};

#include <binder/IBatteryStats.h>

class FakeBatteryStats : public BinderService<FakeBatteryStats>, public BnBatteryStats {
public:
    static char const *getServiceName()
    {
        return "batterystats";
    }
    void noteStartSensor(int uid, int sensor)
    {
    }
    void noteStopSensor(int uid, int sensor)
    {
    }
    void noteStartVideo(int uid)
    {
    }
    void noteStopVideo(int uid)
    {
    }
    void noteStartAudio(int uid)
    {
    }
    void noteStopAudio(int uid)
    {
    }
    void noteResetVideo()
    {
    }
    void noteResetAudio()
    {
    }
    void noteFlashlightOn(int uid)
    {
    }
    void noteFlashlightOff(int uid)
    {
    }
    void noteStartCamera(int uid)
    {
    }
    void noteStopCamera(int uid)
    {
    }
    void noteResetCamera()
    {
    }
    void noteResetFlashlight()
    {
    }
};

#include <gui/ISensorServer.h>
#include <gui/ISensorEventConnection.h>
#include <gui/Sensor.h>
#include <gui/BitTube.h>

class FakeSensorEventConnection : public BnSensorEventConnection {
    sp<BitTube> mChannel;

public:
    FakeSensorEventConnection()
    {
        mChannel = new BitTube(0);
    }
    sp<BitTube> getSensorChannel() const
    {
        return mChannel;
    }
    status_t enableDisable(int handle, bool enabled, nsecs_t samplingPeriodNs,
        nsecs_t maxBatchReportLatencyNs, int reservedFlags)
    {
        return 0;
    }
    status_t setEventRate(int handle, nsecs_t ns)
    {
        return 0;
    }
    status_t flush()
    {
        return 0;
    }
};
class FakeSensorServer : public BinderService<FakeSensorServer>, public BnSensorServer {
public:
    static char const *getServiceName()
    {
        return "sensorservice";
    }

    Vector<Sensor> getSensorList(const String16 &opPackageName)
    {
        return Vector<Sensor>();
    }

    Vector<Sensor> getDynamicSensorList(const String16 &opPackageName)
    {
        return Vector<Sensor>();
    }

    sp<ISensorEventConnection> createSensorEventConnection(
        const String8 &packageName, int mode, const String16 &opPackageName)
    {
        return sp<ISensorEventConnection>(new FakeSensorEventConnection);
    }

    int32_t isDataInjectionEnabled()
    {
        return 0;
    }
};

#include <media/IResourceManagerService.h>
#include <media/IResourceManagerClient.h>
#include <media/MediaResource.h>
#include <media/MediaResourcePolicy.h>

class FakeResourceManagerService : public BinderService<FakeResourceManagerService>,
                                   public BnResourceManagerService {
public:
    static char const *getServiceName()
    {
        return "media.resource_manager";
    }
    void config(const Vector<MediaResourcePolicy> &policies)
    {
    }

    void addResource(int pid, int64_t clientId, const sp<IResourceManagerClient> client,
        const Vector<MediaResource> &resources)
    {
    }

    void removeResource(int pid, int64_t clientId)
    {
    }

    bool reclaimResource(int callingPid, const Vector<MediaResource> &resources)
    {
        return true;
    }
};
