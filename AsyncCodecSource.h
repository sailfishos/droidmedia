/*
 * Copyright (c) 2020 Open Mobile Platform LLC.
 * Copyright 2016, The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef ASYNC_CODEC_SOURCE_H_
#define ASYNC_CODEC_SOURCE_H_

#include <media/openmax/OMX_IVCommon.h>
#include <media/stagefright/foundation/AString.h>
#include <media/stagefright/foundation/Mutexed.h>
#include <media/stagefright/foundation/AHandlerReflector.h>
#if ANDROID_MAJOR >= 9 && ANDROID_MAJOR <= 10
#include <media/MediaSource.h>
#else
#include <media/stagefright/MediaSource.h>
#endif
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaCodecSource.h>
#include <utils/Condition.h>
#include <utils/StrongPointer.h>

struct ANativeWindow;

template<typename T>
struct Buffers {
    android::List<T>buffers;
    android::Condition cond;
    android::Mutex lock;
};

namespace android {

#if ANDROID_MAJOR < 9
typedef MediaBuffer DroidMediaBuffer;
#else
typedef MediaBufferBase DroidMediaBuffer;
#endif

struct ALooper;
struct AMessage;
struct MediaCodec;
class MetaData;
class Surface;

class AsyncCodecSource : public MediaSource {
public:
    static sp<MediaSource> Create(const sp<MediaSource> &source,
                    const sp<AMessage> &format, bool isEncoder, uint32_t flags,
                    const sp<ANativeWindow> &nativeWindow, const sp<ALooper> &looper,
                    const char *desiredCodec = NULL,
                    OMX_COLOR_FORMATTYPE colorFormat = OMX_COLOR_FormatUnused);

    bool configure(const sp<AMessage> format,
                   const sp<Surface> surface,
                   uint32_t flags = 0);

    virtual ~AsyncCodecSource();

    // starts this source (and its underlying source). |params| is ignored.
    virtual status_t start(MetaData *params = NULL);

    // stops this source (and its underlying source).
    virtual status_t stop();

    // returns the output format of this source.
    virtual sp<MetaData> getFormat();

    // reads from the source. This call always blocks.
    virtual status_t read(
                    android::DroidMediaBuffer **buffer,
                    const ReadOptions *options);

    // for AHandlerReflector
    void onMessageReceived(const sp<AMessage> &msg);

    // unsupported methods
    virtual status_t pause() { return INVALID_OPERATION; }

    status_t setParameters(const sp<AMessage> &params);

    void flush();

private:
    class SourceReader : public Thread {
    public:
        explicit SourceReader(AsyncCodecSource *codec, const sp<MediaSource> source);
        void inputBufferAvailable(size_t index);

    private:
        ~SourceReader();
        bool threadLoop() override;
        size_t waitForInputBuffer();

        volatile bool mRunning;
        AsyncCodecSource *mCodec;
        sp<MediaSource> mSource;
        Buffers<size_t> mInputIndices;
    };

    // Construct this using a codec, source and looper.
    AsyncCodecSource(
            const AString &codecName, const sp<MediaSource> &source, const sp<ALooper> &looper,
            bool isVorbis);

    bool queueInputBuffer(DroidMediaBuffer *buffer, size_t inputBufferIndex);
    void queueEOS(size_t index);

    AString mComponentName;
    sp<MediaCodec> mCodec;
    sp<MediaSource> mSource;
    sp<ALooper> mLooper;
    sp<ALooper> mCodecLooper;
    sp<AMessage> mNotify = 0;
    sp<AHandlerReflector<AsyncCodecSource> > mReflector;

    Mutexed<sp<MetaData>> mMeta;
    bool mUsingSurface;
    bool mIsVorbis;
    bool mOutputChanged = false;
    enum State {
        INIT,
        STARTED,
        STOPPING,
        STOPPED,
        ERROR,
    };
    State mState = INIT;

    struct Output {
        Output();

        List<DroidMediaBuffer*> mBufferQueue;
        bool mReachedEOS;
        bool mReading = false;
        Condition mAvailable;
        Condition mReadCondition;
    };
    Mutexed<Output> mOutput;
    sp<SourceReader> mSourceReader;
};

} // namespace android

#endif //ASYNC_DECODING_SOURCE_H_
