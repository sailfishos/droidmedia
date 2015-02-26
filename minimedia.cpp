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
#include <CameraService.h>
#include <binder/MemoryHeapBase.h>
#include <MediaPlayerService.h>
#include <media/IAudioFlinger.h>

using namespace android;

class FakeAudioFlinger : public BinderService<FakeAudioFlinger>,
                         public BnAudioFlinger
{
public:
    static const char* getServiceName() { return "media.audio_flinger"; }

    sp<IAudioTrack> createTrack( pid_t pid, audio_stream_type_t streamType,
                                 uint32_t sampleRate, audio_format_t format,
                                 uint32_t channelMask, int frameCount,
                                 track_flags_t flags, const sp<IMemory>& sharedBuffer,
                                 audio_io_handle_t output, pid_t tid,
                                 int *sessionId, status_t *status) { return NULL; }

    sp<IDirectTrack> createDirectTrack(pid_t pid, uint32_t sampleRate, uint32_t channelMask,
                                       audio_io_handle_t output, int *sessionId,
                                       IDirectTrackClient* client, audio_stream_type_t streamType,
                                       status_t *status) { return NULL; }

    sp<IAudioRecord> openRecord(pid_t pid, audio_io_handle_t input, uint32_t sampleRate,
                                audio_format_t format, uint32_t channelMask,
                                int frameCount, track_flags_t flags,
                                int *sessionId, status_t *status) { return NULL; }

    uint32_t sampleRate(audio_io_handle_t output) const { return 0; }
    int channelCount(audio_io_handle_t output) const {return 0; }
    audio_format_t format(audio_io_handle_t output) const { return AUDIO_FORMAT_INVALID; }
    size_t frameCount(audio_io_handle_t output) const { return 0; }
    uint32_t latency(audio_io_handle_t output) const { return 0; }
    status_t setMasterVolume(float value) { return BAD_VALUE; }
    status_t setMasterMute(bool muted) { return BAD_VALUE; }
    float masterVolume() const { return 0.0; }
    bool masterMute() const {return false; }
    status_t setStreamVolume(audio_stream_type_t stream, float value,
                             audio_io_handle_t output) { return BAD_VALUE; }
    status_t setStreamMute(audio_stream_type_t stream, bool muted) { return BAD_VALUE; }
    float streamVolume(audio_stream_type_t stream, audio_io_handle_t output) const { return 0.0; }
    bool streamMute(audio_stream_type_t stream) const { return false; }
    status_t setMode(audio_mode_t mode) { return BAD_VALUE; }
    status_t setMicMute(bool state) { return BAD_VALUE; }
    bool getMicMute() const { return false; }
    status_t setParameters(audio_io_handle_t ioHandle, const String8& keyValuePairs) { return BAD_VALUE; }
    String8 getParameters(audio_io_handle_t ioHandle, const String8& keys) const { return String8(); }
    void registerClient(const sp<IAudioFlingerClient>& client) {}
    size_t getInputBufferSize(uint32_t sampleRate, audio_format_t format, int channelCount) const { return 0; }
    audio_io_handle_t openOutput(audio_module_handle_t module, audio_devices_t *pDevices,
                                 uint32_t *pSamplingRate, audio_format_t *pFormat,
                                 audio_channel_mask_t *pChannelMask, uint32_t *pLatencyMs,
                                 audio_output_flags_t flags) { return 0; }
    audio_io_handle_t openDuplicateOutput(audio_io_handle_t output1,
                                          audio_io_handle_t output2) { return 0; }
    status_t closeOutput(audio_io_handle_t output) { return BAD_VALUE; }
    status_t suspendOutput(audio_io_handle_t output) { return BAD_VALUE; }
    status_t restoreOutput(audio_io_handle_t output) { return BAD_VALUE; }

    audio_io_handle_t openInput(audio_module_handle_t module, audio_devices_t *pDevices,
                                uint32_t *pSamplingRate, audio_format_t *pFormat,
                                audio_channel_mask_t *pChannelMask) { return 0; }
    status_t closeInput(audio_io_handle_t input) { return BAD_VALUE; }
    status_t setStreamOutput(audio_stream_type_t stream, audio_io_handle_t output) { return BAD_VALUE; }
    status_t setVoiceVolume(float volume) { return BAD_VALUE; }
    status_t getRenderPosition(uint32_t *halFrames, uint32_t *dspFrames,
                               audio_io_handle_t output) const { return BAD_VALUE; }
    unsigned int getInputFramesLost(audio_io_handle_t ioHandle) const { return 0; }
    int newAudioSessionId() { return 0; }
    void acquireAudioSessionId(int audioSession) {}
    void releaseAudioSessionId(int audioSession) {}
    status_t queryNumberEffects(uint32_t *numEffects) const { return BAD_VALUE; }
    status_t queryEffect(uint32_t index, effect_descriptor_t *pDescriptor) const { return BAD_VALUE; }
    status_t getEffectDescriptor(const effect_uuid_t *pEffectUUID,
                                 effect_descriptor_t *pDescriptor) const { return BAD_VALUE; }
    sp<IEffect> createEffect(pid_t pid, effect_descriptor_t *pDesc,
                             const sp<IEffectClient>& client, int32_t priority,
                             audio_io_handle_t output, int sessionId,
                             status_t *status, int *id, int *enabled) { return NULL; }
    status_t moveEffects(int session, audio_io_handle_t srcOutput,
                         audio_io_handle_t dstOutput) { return BAD_VALUE; }
    audio_module_handle_t loadHwModule(const char *name) { return 0; }
    status_t setFmVolume(float volume) { return BAD_VALUE; }
};

int
main(int argc, char* argv[])
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();

    MediaPlayerService::instantiate();
    CameraService::instantiate();
    FakeAudioFlinger::instantiate();

    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
