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
#include <media/IAudioPolicyService.h>
#include <AudioPolicyInterface.h>

#include <unordered_map>

namespace android {

// ----------------------------------------------------------------------------

class FakeAudioPolicyService : public BinderService<FakeAudioPolicyService>,
                               public BnAudioPolicyService,
                               public IBinder::DeathRecipient
{
    friend class BinderService<FakeAudioPolicyService>;

public:
    // for BinderService
    static const char *getServiceName() ANDROID_API { return "media.audio_policy"; }

    virtual status_t dump(int fd, const Vector<String16> &args) { return NO_ERROR; }

    //
    // BnAudioPolicyService (see AudioPolicyInterface for method descriptions)
    //

    void onNewAudioModulesAvailable() override { return; }
    virtual status_t setDeviceConnectionState(audio_devices_t device,
                                              audio_policy_dev_state_t state,
                                              const char *device_address, const char *device_name,
                                              audio_format_t encodedFormat)
    {
        return NO_ERROR;
    }
    virtual audio_policy_dev_state_t getDeviceConnectionState(audio_devices_t device,
                                                              const char *device_address)
    {
        return AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE;
    }
    virtual status_t handleDeviceConfigChange(audio_devices_t device, const char *device_address,
                                              const char *device_name, audio_format_t encodedFormat)
    {
        return NO_ERROR;
    }
    virtual status_t setPhoneState(audio_mode_t state, uid_t uid) { return NO_ERROR; }
    virtual status_t setForceUse(audio_policy_force_use_t usage, audio_policy_forced_cfg_t config)
    {
        return NO_ERROR;
    }
    virtual audio_policy_forced_cfg_t getForceUse(audio_policy_force_use_t usage)
    {
        return AUDIO_POLICY_FORCE_NONE;
    }
    virtual audio_io_handle_t getOutput(audio_stream_type_t stream) { return AUDIO_IO_HANDLE_NONE; }
    status_t getOutputForAttr(audio_attributes_t *attr, audio_io_handle_t *output,
                              audio_session_t session, audio_stream_type_t *stream, pid_t pid,
                              uid_t uid, const audio_config_t *config, audio_output_flags_t flags,
                              audio_port_handle_t *selectedDeviceId, audio_port_handle_t *portId,
                              std::vector<audio_io_handle_t> *secondaryOutputs) override
    {
        return NO_ERROR;
    }
    virtual status_t startOutput(audio_port_handle_t portId) { return NO_ERROR; }
    virtual status_t stopOutput(audio_port_handle_t portId) { return NO_ERROR; }
    virtual void releaseOutput(audio_port_handle_t portId) { return; }
    virtual status_t getInputForAttr(const audio_attributes_t *attr, audio_io_handle_t *input,
                                     audio_unique_id_t riid, audio_session_t session, pid_t pid,
                                     uid_t uid, const String16 &opPackageName,
                                     const audio_config_base_t *config, audio_input_flags_t flags,
                                     audio_port_handle_t *selectedDeviceId = NULL,
                                     audio_port_handle_t *portId = NULL)
    {
        return NO_ERROR;
    }
    virtual status_t startInput(audio_port_handle_t portId) { return NO_ERROR; }
    virtual status_t stopInput(audio_port_handle_t portId) { return NO_ERROR; }
    virtual void releaseInput(audio_port_handle_t portId) { return; }
    virtual status_t initStreamVolume(audio_stream_type_t stream, int indexMin, int indexMax)
    {
        return NO_ERROR;
    }
    virtual status_t setStreamVolumeIndex(audio_stream_type_t stream, int index,
                                          audio_devices_t device)
    {
        return NO_ERROR;
    }
    virtual status_t getStreamVolumeIndex(audio_stream_type_t stream, int *index,
                                          audio_devices_t device)
    {
        return NO_ERROR;
    }

    virtual status_t setVolumeIndexForAttributes(const audio_attributes_t &attr, int index,
                                                 audio_devices_t device)
    {
        return NO_ERROR;
    }
    virtual status_t getVolumeIndexForAttributes(const audio_attributes_t &attr, int &index,
                                                 audio_devices_t device)
    {
        return NO_ERROR;
    }
    virtual status_t getMinVolumeIndexForAttributes(const audio_attributes_t &attr, int &index)
    {
        return NO_ERROR;
    }
    virtual status_t getMaxVolumeIndexForAttributes(const audio_attributes_t &attr, int &index)
    {
        return NO_ERROR;
    }

    virtual uint32_t getStrategyForStream(audio_stream_type_t stream)
    {
        return PRODUCT_STRATEGY_NONE;
    }
    virtual audio_devices_t getDevicesForStream(audio_stream_type_t stream)
    {
        return AUDIO_DEVICE_NONE;
    }
    virtual status_t getDevicesForAttributes(const AudioAttributes &aa,
                                             AudioDeviceTypeAddrVector *devices) const
    {
        return NO_ERROR;
    }

    virtual audio_io_handle_t getOutputForEffect(const effect_descriptor_t *desc) { return 0; }
    virtual status_t registerEffect(const effect_descriptor_t *desc, audio_io_handle_t io,
                                    uint32_t strategy, audio_session_t session, int id)
    {
        return NO_ERROR;
    }
    virtual status_t unregisterEffect(int id) { return NO_ERROR; }
    virtual status_t setEffectEnabled(int id, bool enabled) { return NO_ERROR; }
    status_t moveEffectsToIo(const std::vector<int> &ids, audio_io_handle_t io) override
    {
        return NO_ERROR;
    }
    virtual bool isStreamActive(audio_stream_type_t stream, uint32_t inPastMs = 0) const
    {
        return false;
    }
    virtual bool isStreamActiveRemotely(audio_stream_type_t stream, uint32_t inPastMs = 0) const
    {
        return false;
    }
    virtual bool isSourceActive(audio_source_t source) const { return false; }

    virtual status_t queryDefaultPreProcessing(audio_session_t audioSession,
                                               effect_descriptor_t *descriptors, uint32_t *count)
    {
        return NO_ERROR;
    }
    virtual status_t addSourceDefaultEffect(const effect_uuid_t *type,
                                            const String16 &opPackageName,
                                            const effect_uuid_t *uuid, int32_t priority,
                                            audio_source_t source, audio_unique_id_t *id)
    {
        return NO_ERROR;
    }
    virtual status_t addStreamDefaultEffect(const effect_uuid_t *type,
                                            const String16 &opPackageName,
                                            const effect_uuid_t *uuid, int32_t priority,
                                            audio_usage_t usage, audio_unique_id_t *id)
    {
        return NO_ERROR;
    }
    virtual status_t removeSourceDefaultEffect(audio_unique_id_t id) { return NO_ERROR; }
    virtual status_t removeStreamDefaultEffect(audio_unique_id_t id) { return NO_ERROR; }

    virtual status_t onTransact(uint32_t code, const Parcel &data, Parcel *reply, uint32_t flags)
    {
        return NO_ERROR;
    }

    // IBinder::DeathRecipient
    virtual void binderDied(const wp<IBinder> &who) { return; }

    // RefBase
    virtual void onFirstRef() { return; }

    //
    // Helpers for the struct audio_policy_service_ops implementation.
    // This is used by the audio policy manager for certain operations that
    // are implemented by the policy service.
    //
    virtual void setParameters(audio_io_handle_t ioHandle, const char *keyValuePairs, int delayMs)
    {
        return;
    }

    virtual status_t setStreamVolume(audio_stream_type_t stream, float volume,
                                     audio_io_handle_t output, int delayMs = 0)
    {
        return NO_ERROR;
    }
    virtual status_t setVoiceVolume(float volume, int delayMs = 0) { return NO_ERROR; }
    status_t setSupportedSystemUsages(const std::vector<audio_usage_t> &systemUsages)
    {
        return NO_ERROR;
    }
    status_t setAllowedCapturePolicy(uint_t uid, audio_flags_mask_t capturePolicy) override
    {
        return NO_ERROR;
    }
    virtual bool isOffloadSupported(const audio_offload_info_t &config) { return false; }
    virtual bool isDirectOutputSupported(const audio_config_base_t &config,
                                         const audio_attributes_t &attributes)
    {
        return false;
    }

    virtual status_t listAudioPorts(audio_port_role_t role, audio_port_type_t type,
                                    unsigned int *num_ports, struct audio_port *ports,
                                    unsigned int *generation)
    {
        return NO_ERROR;
    }
    virtual status_t getAudioPort(struct audio_port *port) { return NO_ERROR; }
    virtual status_t createAudioPatch(const struct audio_patch *patch, audio_patch_handle_t *handle)
    {
        return NO_ERROR;
    }
    virtual status_t releaseAudioPatch(audio_patch_handle_t handle) { return NO_ERROR; }
    virtual status_t listAudioPatches(unsigned int *num_patches, struct audio_patch *patches,
                                      unsigned int *generation)
    {
        return NO_ERROR;
    }
    virtual status_t setAudioPortConfig(const struct audio_port_config *config) { return NO_ERROR; }

    virtual void registerClient(const sp<IAudioPolicyServiceClient> &client) { return; }

    virtual void setAudioPortCallbacksEnabled(bool enabled) { return; }

    virtual void setAudioVolumeGroupCallbacksEnabled(bool enabled) { return; }

    virtual status_t acquireSoundTriggerSession(audio_session_t *session,
                                                audio_io_handle_t *ioHandle,
                                                audio_devices_t *device)
    {
        return NO_ERROR;
    }

    virtual status_t releaseSoundTriggerSession(audio_session_t session) { return NO_ERROR; }

    virtual audio_mode_t getPhoneState() { return AUDIO_MODE_CURRENT; }

    virtual status_t registerPolicyMixes(const Vector<AudioMix> &mixes, bool registration)
    {
        return NO_ERROR;
    }

    virtual status_t setUidDeviceAffinities(uid_t uid, const Vector<AudioDeviceTypeAddr> &devices)
    {
        return NO_ERROR;
    }

    virtual status_t removeUidDeviceAffinities(uid_t uid) { return NO_ERROR; }

    virtual status_t setPreferredDeviceForStrategy(product_strategy_t strategy,
                                                   const AudioDeviceTypeAddr &device)
    {
        return NO_ERROR;
    }

    virtual status_t removePreferredDeviceForStrategy(product_strategy_t strategy)
    {
        return NO_ERROR;
    }

    virtual status_t getPreferredDeviceForStrategy(product_strategy_t strategy,
                                                   AudioDeviceTypeAddr &device)
    {
        return NO_ERROR;
    }
    virtual status_t setUserIdDeviceAffinities(int userId,
                                               const Vector<AudioDeviceTypeAddr> &devices)
    {
        return NO_ERROR;
    }

    virtual status_t removeUserIdDeviceAffinities(int userId) { return NO_ERROR; }

    virtual status_t startAudioSource(const struct audio_port_config *source,
                                      const audio_attributes_t *attributes,
                                      audio_port_handle_t *portId)
    {
        return NO_ERROR;
    }
    virtual status_t stopAudioSource(audio_port_handle_t portId) { return NO_ERROR; }

    virtual status_t setMasterMono(bool mono) { return NO_ERROR; }
    virtual status_t getMasterMono(bool *mono) { return NO_ERROR; }

    virtual float getStreamVolumeDB(audio_stream_type_t stream, int index, audio_devices_t device)
    {
        return 0;
    }

    virtual status_t getSurroundFormats(unsigned int *numSurroundFormats,
                                        audio_format_t *surroundFormats,
                                        bool *surroundFormatsEnabled, bool reported)
    {
        return NO_ERROR;
    }
    virtual status_t
    getHwOffloadEncodingFormatsSupportedForA2DP(std::vector<audio_format_t> *formats)
    {
        return NO_ERROR;
    }
    virtual status_t setSurroundFormatEnabled(audio_format_t audioFormat, bool enabled)
    {
        return NO_ERROR;
    }

    virtual status_t setAssistantUid(uid_t uid) { return NO_ERROR; }
    virtual status_t setA11yServicesUids(const std::vector<uid_t> &uids) { return NO_ERROR; }

    virtual status_t setCurrentImeUid(uid_t uid) { return NO_ERROR; }

    virtual bool isHapticPlaybackSupported() { return false; }

    virtual status_t listAudioProductStrategies(AudioProductStrategyVector &strategies)
    {
        return NO_ERROR;
    }
    virtual status_t getProductStrategyFromAudioAttributes(const AudioAttributes &aa,
                                                           product_strategy_t &productStrategy)
    {
        return NO_ERROR;
    }

    virtual status_t listAudioVolumeGroups(AudioVolumeGroupVector &groups) { return NO_ERROR; }

    virtual status_t getVolumeGroupFromAudioAttributes(const AudioAttributes &aa,
                                                       volume_group_t &volumeGroup)
    {
        return NO_ERROR;
    }

    status_t
    registerSoundTriggerCaptureStateListener(const sp<media::ICaptureStateListener> &listener,
                                             bool *result) override
    {
        return NO_ERROR;
    }

    virtual status_t setRttEnabled(bool enabled) { return NO_ERROR; }

    bool isCallScreenModeSupported() override { return false; }

    void doOnNewAudioModulesAvailable() { return; }

    status_t doStopOutput(audio_port_handle_t portId) { return NO_ERROR; }
    void doReleaseOutput(audio_port_handle_t portId) { return; }

    status_t clientCreateAudioPatch(const struct audio_patch *patch, audio_patch_handle_t *handle,
                                    int delayMs)
    {
        return NO_ERROR;
    }
    status_t clientReleaseAudioPatch(audio_patch_handle_t handle, int delayMs) { return NO_ERROR; }
    virtual status_t clientSetAudioPortConfig(const struct audio_port_config *config, int delayMs)
    {
        return NO_ERROR;
    }

    void removeNotificationClient(uid_t uid, pid_t pid) { return; }
    void onAudioPortListUpdate() { return; }
    void doOnAudioPortListUpdate() { return; }
    void onAudioPatchListUpdate() { return; }
    void doOnAudioPatchListUpdate() { return; }

    void onDynamicPolicyMixStateUpdate(const String8 &regId, int32_t state) { return; }
    void doOnDynamicPolicyMixStateUpdate(const String8 &regId, int32_t state) { return; }
    void onRecordingConfigurationUpdate(int event, const record_client_info_t *clientInfo,
                                        const audio_config_base_t *clientConfig,
                                        std::vector<effect_descriptor_t> clientEffects,
                                        const audio_config_base_t *deviceConfig,
                                        std::vector<effect_descriptor_t> effects,
                                        audio_patch_handle_t patchHandle, audio_source_t source)
    {
        return;
    }
    void doOnRecordingConfigurationUpdate(int event, const record_client_info_t *clientInfo,
                                          const audio_config_base_t *clientConfig,
                                          std::vector<effect_descriptor_t> clientEffects,
                                          const audio_config_base_t *deviceConfig,
                                          std::vector<effect_descriptor_t> effects,
                                          audio_patch_handle_t patchHandle, audio_source_t source)
    {
        return;
    }

    void onAudioVolumeGroupChanged(volume_group_t group, int flags) { return; }
    void doOnAudioVolumeGroupChanged(volume_group_t group, int flags) { return; }
    void setEffectSuspended(int effectId, audio_session_t sessionId, bool suspended) { return; }

#ifdef AUDIOPOLICY_LINEAGE_AUDIOSESSIONINFO
    status_t listAudioSessions(audio_stream_type_t streams, Vector<sp<AudioSessionInfo>> &sessions)
    {
        return NO_ERROR;
    }
#endif
};

} // namespace android

#endif // FAKE_ANDROID_AUDIOPOLICYSERVICE_H
