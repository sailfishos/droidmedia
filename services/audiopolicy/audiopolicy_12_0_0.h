/* -*- c++ -*-
 * Copyright (C) 2009 The Android Open Source Project
 * Copyright (C) 2022 Jolla Ltd.
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
 */

#ifndef FAKE_ANDROID_AUDIOPOLICYSERVICE_H
#define FAKE_ANDROID_AUDIOPOLICYSERVICE_H

#include <cutils/misc.h>
#include <cutils/config_utils.h>
#include <cutils/compiler.h>
#include <utils/String8.h>
#include <utils/Vector.h>
#include <utils/SortedVector.h>
#include <binder/BinderService.h>
#include <binder/IUidObserver.h>
#include <system/audio.h>
#include <system/audio_policy.h>
#include <android/media/BnAudioPolicyService.h>
#include <Spatializer.h>

#include <unordered_map>


namespace android {

// ----------------------------------------------------------------------------

class FakeAudioPolicyService :
    public BinderService<FakeAudioPolicyService>,
    public media::BnAudioPolicyService,
    public IBinder::DeathRecipient,
    public SpatializerPolicyCallback
{
    friend class BinderService<FakeAudioPolicyService>;

public:
    // for BinderService
    static const char *getServiceName() ANDROID_API { return "media.audio_policy"; }

    status_t    dump(int fd, const Vector<String16>& args) { return NO_ERROR; }

    //
    // BnAudioPolicyService (see AudioPolicyInterface for method descriptions)
    //
    ::binder::Status onNewAudioModulesAvailable() override {
        return ::binder::Status::ok();
    }
    binder::Status setDeviceConnectionState(const media::AudioDevice&,
                                            media::AudioPolicyDeviceState,
                                            const std::string&,
                                            media::audio::common::AudioFormat) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getDeviceConnectionState(const media::AudioDevice &,
                                              media::AudioPolicyDeviceState *state) override {
        *state = ::media::AudioPolicyDeviceState::UNAVAILABLE;
        return ::binder::Status::ok();
    }
    ::binder::Status handleDeviceConfigChange(const media::AudioDevice &,
                                              const std::string &,
                                              media::audio::common::AudioFormat) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setPhoneState(media::AudioMode state, int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setForceUse(media::AudioPolicyForceUse,
                                 media::AudioPolicyForcedConfig) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getForceUse(media::AudioPolicyForceUse,
                                 media::AudioPolicyForcedConfig *config) override {
        *config = media::AudioPolicyForcedConfig::NONE;
        return ::binder::Status::ok();
    }
    ::binder::Status getOutput(media::AudioStreamType,
                               int32_t *output) override {
        *output = 0;
        return ::binder::Status::ok();
    }
    ::binder::Status getOutputForAttr(const media::AudioAttributesInternal &,
                                      int32_t,
                                      const AttributionSourceState &,
                                      const media::AudioConfig &,
                                      int32_t,
                                      int32_t,
                                      media::GetOutputForAttrResponse *output) override {
        return ::binder::Status::ok();
    }
    ::binder::Status startOutput(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status stopOutput(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status releaseOutput(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getInputForAttr(const media::AudioAttributesInternal &,
                                     int32_t,
                                     int32_t,
                                     int32_t,
                                     const AttributionSourceState &,
                                     const media::AudioConfigBase &,
                                     int32_t,
                                     int32_t,
                                     media::GetInputForAttrResponse *input) override {
        return ::binder::Status::ok();
    }
    ::binder::Status startInput(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status stopInput(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status releaseInput(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status initStreamVolume(media::AudioStreamType,
                                      int32_t,
                                      int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setStreamVolumeIndex(media::AudioStreamType stream,
                                          int32_t,
                                          int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getStreamVolumeIndex(media::AudioStreamType,
                                          int32_t ,
                                          int32_t *) override {
        return ::binder::Status::ok();
    }

    ::binder::Status setVolumeIndexForAttributes(const media::AudioAttributesInternal &,
                                                 int32_t device,
                                                 int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getVolumeIndexForAttributes(const media::AudioAttributesInternal &,
                                                 int32_t,
                                                 int32_t *) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getMinVolumeIndexForAttributes(const media::AudioAttributesInternal &,
                                                    int32_t *) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getMaxVolumeIndexForAttributes(const media::AudioAttributesInternal &,
                                                    int32_t *) override {
        return ::binder::Status::ok();
    }

    ::binder::Status getStrategyForStream(media::AudioStreamType stream,
                                          int32_t *strategy) override {
        *strategy = PRODUCT_STRATEGY_NONE;
        return ::binder::Status::ok();
    }
    ::binder::Status getDevicesForStream(media::AudioStreamType stream,
                                         int32_t* devices) override {
        *devices = 0;
        return ::binder::Status::ok();
    }
    ::binder::Status getDevicesForAttributes(const media::AudioAttributesEx&,
                                             std::vector<media::AudioDevice>* devices) override {
        return ::binder::Status::ok();
    }

    ::binder::Status getOutputForEffect(const media::EffectDescriptor &,
                                        int32_t *output) override {
        *output = 0;
        return ::binder::Status::ok();
    }
    ::binder::Status registerEffect(const media::EffectDescriptor &,
                                    int32_t,
                                    int32_t,
                                    int32_t,
                                    int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status unregisterEffect(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setEffectEnabled(int32_t,
                                      bool) override {
        return ::binder::Status::ok();
    }
    ::binder::Status moveEffectsToIo(const std::vector<int32_t> &,
                                     int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status isStreamActive(media::AudioStreamType,
                                    int32_t,
                                    bool *active) override {
        *active = false;
        return ::binder::Status::ok();
    }
    ::binder::Status isStreamActiveRemotely(media::AudioStreamType,
                                            int32_t,
                                            bool *active) override {
        *active = false;
        return ::binder::Status::ok();
    }
    ::binder::Status isSourceActive(media::AudioSourceType,
                                    bool *active) override {
        *active = false;
        return ::binder::Status::ok();
    }
    ::binder::Status queryDefaultPreProcessing(int32_t audioSession,
                                               media::Int*,
                                               std::vector<media::EffectDescriptor>*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status addSourceDefaultEffect(const media::AudioUuid &,
                                            const std::string &,
                                            const media::AudioUuid &,
                                            int32_t,
                                            media::AudioSourceType,
                                            int32_t *) override {
        return ::binder::Status::ok();
    }
    ::binder::Status addStreamDefaultEffect(const media::AudioUuid &,
                                            const std::string &,
                                            const media::AudioUuid &,
                                            int32_t,
                                            media::AudioUsage,
                                            int32_t *) override {
        return ::binder::Status::ok();
    }
    ::binder::Status removeSourceDefaultEffect(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status removeStreamDefaultEffect(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setSupportedSystemUsages(const std::vector<media::AudioUsage>&) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setAllowedCapturePolicy(int32_t,
                                             int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getOffloadSupport(const media::AudioOffloadInfo& info,
                                       media::AudioOffloadMode*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status isDirectOutputSupported(const media::AudioConfigBase& config,
                                             const media::AudioAttributesInternal& attributes,
                                             bool*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status listAudioPorts(media::AudioPortRole,
                                    media::AudioPortType,
                                    media::Int*,
                                    std::vector<media::AudioPort>*,
                                    int32_t*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getAudioPort(const media::AudioPort&,
                                  media::AudioPort*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status createAudioPatch(const media::AudioPatch&,
                                      int32_t,
                                      int32_t*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status releaseAudioPatch(int32_t handle) override {
        return ::binder::Status::ok();
    }
    ::binder::Status listAudioPatches(media::Int*,
                                      std::vector<media::AudioPatch>*,
                                      int32_t*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setAudioPortConfig(const media::AudioPortConfig&) override {
        return ::binder::Status::ok();
    }
    ::binder::Status registerClient(const sp<media::IAudioPolicyServiceClient>&) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setAudioPortCallbacksEnabled(bool) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setAudioVolumeGroupCallbacksEnabled(bool) override {
        return ::binder::Status::ok();
    }
    ::binder::Status acquireSoundTriggerSession(media::SoundTriggerSession*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status releaseSoundTriggerSession(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getPhoneState(media::AudioMode*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status registerPolicyMixes(const std::vector<media::AudioMix>& mixes,
                                         bool registration) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setUidDeviceAffinities(int32_t uid,
                                            const std::vector<media::AudioDevice>& devices) override {
        return ::binder::Status::ok();
    }
    ::binder::Status removeUidDeviceAffinities(int32_t uid) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setUserIdDeviceAffinities(int32_t userId,
                                               const std::vector<media::AudioDevice>& devices) override {
        return ::binder::Status::ok();
    }
    ::binder::Status removeUserIdDeviceAffinities(int32_t userId) override {
        return ::binder::Status::ok();
    }
    ::binder::Status startAudioSource(const media::AudioPortConfig&,
                                      const media::AudioAttributesInternal&,
                                      int32_t*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status stopAudioSource(int32_t portId) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setMasterMono(bool mono) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getMasterMono(bool*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getStreamVolumeDB(media::AudioStreamType, int32_t, int32_t,
                                       float* _aidl_return) override {
        *_aidl_return = 0.0f;
        return ::binder::Status::ok();
    }
    ::binder::Status getSurroundFormats(media::Int*,
                                        std::vector<media::audio::common::AudioFormat>*,
                                        std::vector<bool>*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getReportedSurroundFormats(media::Int*,
                                                std::vector<media::audio::common::AudioFormat>*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getHwOffloadEncodingFormatsSupportedForA2DP(
            std::vector<media::audio::common::AudioFormat>*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setSurroundFormatEnabled(media::audio::common::AudioFormat audioFormat,
                                              bool enabled) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setAssistantUid(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setHotwordDetectionServiceUid(int32_t)  override {
        return ::binder::Status::ok();
    }
    ::binder::Status setA11yServicesUids(const std::vector<int32_t>&) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setCurrentImeUid(int32_t) override {
        return ::binder::Status::ok();
    }
    ::binder::Status isHapticPlaybackSupported(bool*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status listAudioProductStrategies(
        std::vector<media::AudioProductStrategy>*) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getProductStrategyFromAudioAttributes(const media::AudioAttributesEx& aa,
                                                           bool fallbackOnDefault,
                                                           int32_t* _aidl_return) override {
        return ::binder::Status::ok();
    }
    ::binder::Status listAudioVolumeGroups(std::vector<media::AudioVolumeGroup>* _aidl_return) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getVolumeGroupFromAudioAttributes(const media::AudioAttributesEx& aa,
                                                       bool fallbackOnDefault,
                                                       int32_t* _aidl_return) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setRttEnabled(bool enabled) override {
        return ::binder::Status::ok();
    }
    ::binder::Status isCallScreenModeSupported(bool* _aidl_return) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setDevicesRoleForStrategy(int32_t strategy,
                                               media::DeviceRole role,
                                               const std::vector<media::AudioDevice>& devices) override {
        return ::binder::Status::ok();
    }
    ::binder::Status removeDevicesRoleForStrategy(int32_t strategy,
                                                  media::DeviceRole role) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getDevicesForRoleAndStrategy(int32_t strategy,
                                                  media::DeviceRole role,
                                                  std::vector<media::AudioDevice>* _aidl_return) override {
        return ::binder::Status::ok();
    }
    ::binder::Status setDevicesRoleForCapturePreset(media::AudioSourceType audioSource,
                                                    media::DeviceRole role,
                                                    const std::vector<media::AudioDevice>& devices) override {
        return ::binder::Status::ok();
    }
    ::binder::Status addDevicesRoleForCapturePreset(media::AudioSourceType audioSource,
                                                    media::DeviceRole role,
                                                    const std::vector<media::AudioDevice>& devices) override {
        return ::binder::Status::ok();
    }
    ::binder::Status removeDevicesRoleForCapturePreset(media::AudioSourceType audioSource,
                                                       media::DeviceRole role,
                                                       const std::vector<media::AudioDevice>& devices) override {
        return ::binder::Status::ok();
    }
    ::binder::Status clearDevicesRoleForCapturePreset(media::AudioSourceType audioSource,
                                                      media::DeviceRole role) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getDevicesForRoleAndCapturePreset(media::AudioSourceType audioSource,
                                                       media::DeviceRole role,
                                                       std::vector<media::AudioDevice>* _aidl_return) override {
        return ::binder::Status::ok();
    }
    ::binder::Status registerSoundTriggerCaptureStateListener(const sp<media::ICaptureStateListener>& listener,
                                                              bool* _aidl_return) override {
        return ::binder::Status::ok();
    }
    ::binder::Status getSpatializer(const sp<media::INativeSpatializerCallback>& callback,
                                    media::GetSpatializerResponse* _aidl_return) override {
        return ::binder::Status::ok();
    }
    ::binder::Status canBeSpatialized(const std::optional<media::AudioAttributesInternal>& attr,
                                      const std::optional<media::AudioConfig>& config,
                                      const std::vector<media::AudioDevice>& devices,
                                      bool* _aidl_return) override {
        return ::binder::Status::ok();
    }

    status_t onTransact(uint32_t code,
                        const Parcel& data,
                        Parcel* reply,
                        uint32_t flags) override {
        return NO_ERROR;
    }

    // IBinder::DeathRecipient
        void        binderDied(const wp<IBinder>& who) {
        return;
    }

    // RefBase
        void        onFirstRef() {
        return;
    }

    //
    // Helpers for the struct audio_policy_service_ops implementation.
    // This is used by the audio policy manager for certain operations that
    // are implemented by the policy service.
    //
    virtual void setParameters(audio_io_handle_t ioHandle,
                               const char *keyValuePairs,
                               int delayMs) {
        return;
    }

    virtual status_t setStreamVolume(audio_stream_type_t stream,
                                     float volume,
                                     audio_io_handle_t output,
                                     int delayMs = 0) {
        return NO_ERROR;
    }
    virtual status_t setVoiceVolume(float volume, int delayMs = 0) {
        return NO_ERROR;
    }

    void doOnNewAudioModulesAvailable() {
        return;
    }

    status_t doStopOutput(audio_port_handle_t portId) {
        return NO_ERROR;
    }
    void doReleaseOutput(audio_port_handle_t portId) {
        return;
    }

    status_t clientCreateAudioPatch(const struct audio_patch *patch,
                                    audio_patch_handle_t *handle,
                                    int delayMs) {
        return NO_ERROR;
    }
    status_t clientReleaseAudioPatch(audio_patch_handle_t handle,
                                     int delayMs) {
        return NO_ERROR;
    }
    virtual status_t clientSetAudioPortConfig(const struct audio_port_config *config,
                                              int delayMs) {
        return NO_ERROR;
    }

    void removeNotificationClient(uid_t uid, pid_t pid) { return; }
    void onAudioPortListUpdate() { return; }
    void doOnAudioPortListUpdate() { return; }
    void onAudioPatchListUpdate() { return; }
    void doOnAudioPatchListUpdate() { return; }

    void onDynamicPolicyMixStateUpdate(const String8& regId, int32_t state) {
        return;
    }
    void doOnDynamicPolicyMixStateUpdate(const String8& regId, int32_t state) {
        return;
    }
    void onRecordingConfigurationUpdate(int event,
                                        const record_client_info_t *clientInfo,
                                        const audio_config_base_t *clientConfig,
                                        std::vector<effect_descriptor_t> clientEffects,
                                        const audio_config_base_t *deviceConfig,
                                        std::vector<effect_descriptor_t> effects,
                                        audio_patch_handle_t patchHandle,
                                        audio_source_t source) {
        return;
    }
    void doOnRecordingConfigurationUpdate(int event,
                                          const record_client_info_t *clientInfo,
                                          const audio_config_base_t *clientConfig,
                                          std::vector<effect_descriptor_t> clientEffects,
                                          const audio_config_base_t *deviceConfig,
                                          std::vector<effect_descriptor_t> effects,
                                          audio_patch_handle_t patchHandle,
                                          audio_source_t source) {
        return;
    }

    void onAudioVolumeGroupChanged(volume_group_t group, int flags) {
        return;
    }
    void doOnAudioVolumeGroupChanged(volume_group_t group, int flags) {
        return;
    }
    void onRoutingUpdated() {
        return;
    }
    void doOnRoutingUpdated() {
        return;
    }
    void onVolumeRangeInitRequest() {
        return;
    }
    void doOnVolumeRangeInitRequest() {
        return;
    }
    void onCheckSpatializer() {
        return;
    }
    void onCheckSpatializer_l() {
        return;
    }
    void doOnCheckSpatializer() {
        return;
    }
    void onUpdateActiveSpatializerTracks_l() {
        return;
    }
    void doOnUpdateActiveSpatializerTracks() {
        return;
    }

    void setEffectSuspended(int effectId,
                            audio_session_t sessionId,
                            bool suspended) {
        return;
    }
};

}

#endif // FAKE_ANDROID_AUDIOPOLICYSERVICE_H
