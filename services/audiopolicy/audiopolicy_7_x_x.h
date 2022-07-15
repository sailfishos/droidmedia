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
#include <system/audio.h>
#include <system/audio_policy.h>
#include <hardware/audio_policy.h>
#include <media/IAudioPolicyService.h>


namespace android {

// ----------------------------------------------------------------------------

class FakeAudioPolicyService :
    public BinderService<FakeAudioPolicyService>,
    public BnAudioPolicyService,
    public IBinder::DeathRecipient
{
    friend class BinderService<FakeAudioPolicyService>;

public:
    // for BinderService
    static const char *getServiceName() ANDROID_API { return "media.audio_policy"; }

    virtual status_t    dump(int fd, const Vector<String16>& args) { return NO_ERROR; }

    //
    // BnAudioPolicyService (see AudioPolicyInterface for method descriptions)
    //

    virtual status_t setDeviceConnectionState(audio_devices_t device,
                                              audio_policy_dev_state_t state,
                                              const char *device_address,
                                              const char *device_name) {
        return NO_ERROR;
    }
    virtual audio_policy_dev_state_t getDeviceConnectionState(audio_devices_t device,
                                                              const char *device_address) {
        return AUDIO_POLICY_DEVICE_STATE_UNAVAILABLE;
    }
    virtual status_t setPhoneState(audio_mode_t state) { return NO_ERROR; }
    virtual status_t setForceUse(audio_policy_force_use_t usage,
                                 audio_policy_forced_cfg_t config) {
        return NO_ERROR;
    }
    virtual audio_policy_forced_cfg_t getForceUse(audio_policy_force_use_t usage) {
        return AUDIO_POLICY_FORCE_NONE;
    }
    virtual audio_io_handle_t getOutput(audio_stream_type_t stream,
                                        uint32_t samplingRate = 0,
                                        audio_format_t format = AUDIO_FORMAT_DEFAULT,
                                        audio_channel_mask_t channelMask = 0,
                                        audio_output_flags_t flags =
                                                AUDIO_OUTPUT_FLAG_NONE,
                                        const audio_offload_info_t *offloadInfo = NULL) {
        return AUDIO_IO_HANDLE_NONE;
    }
    virtual status_t getOutputForAttr(const audio_attributes_t *attr,
                                      audio_io_handle_t *output,
                                      audio_session_t session,
                                      audio_stream_type_t *stream,
                                      uid_t uid,
                                      uint32_t samplingRate = 0,
                                      audio_format_t format = AUDIO_FORMAT_DEFAULT,
                                      audio_channel_mask_t channelMask = 0,
                                      audio_output_flags_t flags = AUDIO_OUTPUT_FLAG_NONE,
                                      audio_port_handle_t selectedDeviceId = AUDIO_PORT_HANDLE_NONE,
                                      const audio_offload_info_t *offloadInfo = NULL) {
        return NO_ERROR;
    }
    virtual status_t startOutput(audio_io_handle_t output,
                                 audio_stream_type_t stream,
                                 audio_session_t session) {
        return NO_ERROR;
    }
    virtual status_t stopOutput(audio_io_handle_t output,
                                audio_stream_type_t stream,
                                audio_session_t session) {
        return NO_ERROR;
    }
    virtual void releaseOutput(audio_io_handle_t output,
                               audio_stream_type_t stream,
                               audio_session_t session) {
        return;
    }
    virtual status_t getInputForAttr(const audio_attributes_t *attr,
                                     audio_io_handle_t *input,
                                     audio_session_t session,
                                     pid_t pid,
                                     uid_t uid,
                                     uint32_t samplingRate,
                                     audio_format_t format,
                                     audio_channel_mask_t channelMask,
                                     audio_input_flags_t flags,
                                     audio_port_handle_t selectedDeviceId = AUDIO_PORT_HANDLE_NONE) {
        return NO_ERROR;
    }
    virtual status_t startInput(audio_io_handle_t input,
                                audio_session_t session) {
        return NO_ERROR;
    }
    virtual status_t stopInput(audio_io_handle_t input,
                               audio_session_t session) {
        return NO_ERROR;
    }
    virtual void releaseInput(audio_io_handle_t input,
                              audio_session_t session) {
        return;
    }
    virtual status_t initStreamVolume(audio_stream_type_t stream,
                                      int indexMin,
                                      int indexMax) {
        return NO_ERROR;
    }
    virtual status_t setStreamVolumeIndex(audio_stream_type_t stream,
                                          int index,
                                          audio_devices_t device) {
        return NO_ERROR;
    }
    virtual status_t getStreamVolumeIndex(audio_stream_type_t stream,
                                          int *index,
                                          audio_devices_t device) {
        return NO_ERROR;
    }
    virtual uint32_t getStrategyForStream(audio_stream_type_t stream)  {
        return 0;
    }
    virtual audio_devices_t getDevicesForStream(audio_stream_type_t stream) {
        return AUDIO_DEVICE_NONE;
    }
    virtual audio_io_handle_t getOutputForEffect(const effect_descriptor_t *desc) {
        return 0;
    }
    virtual status_t registerEffect(const effect_descriptor_t *desc,
                                    audio_io_handle_t io,
                                    uint32_t strategy,
                                    audio_session_t session,
                                    int id) {
        return NO_ERROR;
    }
    virtual status_t unregisterEffect(int id) { return NO_ERROR; }
    virtual status_t setEffectEnabled(int id, bool enabled) { return NO_ERROR; }
    virtual bool isStreamActive(audio_stream_type_t stream,
                                uint32_t inPastMs = 0) const {
        return false;
    }
    virtual bool isStreamActiveRemotely(audio_stream_type_t stream,
                                        uint32_t inPastMs = 0) const {
        return false;
    }
    virtual bool isSourceActive(audio_source_t source) const { return false; }
    virtual status_t queryDefaultPreProcessing(audio_session_t audioSession,
                                              effect_descriptor_t *descriptors,
                                              uint32_t *count) {
        return NO_ERROR;
    }
    virtual     status_t    onTransact(
                                uint32_t code,
                                const Parcel& data,
                                Parcel* reply,
                                uint32_t flags) {
        return NO_ERROR;
    }
    // IBinder::DeathRecipient
    virtual     void        binderDied(const wp<IBinder>& who) { return; }

    // RefBase
    virtual     void        onFirstRef() { return; }

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
    virtual status_t startTone(audio_policy_tone_t tone, audio_stream_type_t stream) {
        return NO_ERROR;
    }
    virtual status_t stopTone() { return NO_ERROR; }
    virtual status_t setVoiceVolume(float volume, int delayMs = 0) {
        return NO_ERROR;
    }
    virtual bool isOffloadSupported(const audio_offload_info_t &config) {
        return false;
    }
    virtual status_t listAudioPorts(audio_port_role_t role,
                                    audio_port_type_t type,
                                    unsigned int *num_ports,
                                    struct audio_port *ports,
                                    unsigned int *generation) {
        return NO_ERROR;
    }
    virtual status_t getAudioPort(struct audio_port *port) {
        return NO_ERROR;
    }
    virtual status_t createAudioPatch(const struct audio_patch *patch,
                                       audio_patch_handle_t *handle) {
        return NO_ERROR;
    }
    virtual status_t releaseAudioPatch(audio_patch_handle_t handle) {
        return NO_ERROR;
    }
    virtual status_t listAudioPatches(unsigned int *num_patches,
                                      struct audio_patch *patches,
                                      unsigned int *generation) {
        return NO_ERROR;
    }
    virtual status_t setAudioPortConfig(const struct audio_port_config *config) {
        return NO_ERROR;
    }
    virtual void registerClient(const sp<IAudioPolicyServiceClient>& client) {
        return;
    }
    virtual void setAudioPortCallbacksEnabled(bool enabled) { return; }
    virtual status_t acquireSoundTriggerSession(audio_session_t *session,
                                           audio_io_handle_t *ioHandle,
                                           audio_devices_t *device) {
        return NO_ERROR;
    }
    virtual status_t releaseSoundTriggerSession(audio_session_t session) {
        return NO_ERROR;
    }
    virtual audio_mode_t getPhoneState() {
        return AUDIO_MODE_CURRENT;
    }
    virtual status_t registerPolicyMixes(Vector<AudioMix> mixes, bool registration) {
        return NO_ERROR;
    }
    virtual status_t startAudioSource(const struct audio_port_config *source,
                                      const audio_attributes_t *attributes,
                                      audio_io_handle_t *handle) {
        return NO_ERROR;
    }
    virtual status_t stopAudioSource(audio_io_handle_t handle) { return NO_ERROR; }
    virtual status_t setMasterMono(bool mono) { return NO_ERROR; }
    virtual status_t getMasterMono(bool *mono) { return NO_ERROR; }
    status_t doStopOutput(audio_io_handle_t output,
                          audio_stream_type_t stream,
                          audio_session_t session) {
        return NO_ERROR;
    }
    void doReleaseOutput(audio_io_handle_t output,
                         audio_stream_type_t stream,
                         audio_session_t session) {
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
    void removeNotificationClient(uid_t uid) { return; }
    void onAudioPortListUpdate() { return; }
    void doOnAudioPortListUpdate() { return; }
    void onAudioPatchListUpdate() { return; }
    void doOnAudioPatchListUpdate() { return; }
    void onDynamicPolicyMixStateUpdate(String8 regId, int32_t state) {
        return;
    }
    void doOnDynamicPolicyMixStateUpdate(String8 regId, int32_t state) {
        return;
    }
    void onRecordingConfigurationUpdate(int event, audio_session_t session,
                                        audio_source_t source,
                                        const audio_config_base_t *clientConfig,
                                        const audio_config_base_t *deviceConfig,
                                        audio_patch_handle_t patchHandle) {
        return;
    }
    void doOnRecordingConfigurationUpdate(int event, audio_session_t session,
                                          audio_source_t source,
                                          const audio_config_base_t *clientConfig,
                                          const audio_config_base_t *deviceConfig,
                                          audio_patch_handle_t patchHandle) {
        return;
    }
};

}; // namespace android
#endif // FAKE_ANDROID_AUDIOPOLICYSERVICE_H
