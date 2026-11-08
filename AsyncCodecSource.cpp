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

//#define LOG_NDEBUG 0
#include "AsyncCodecSource.h"
#include <utils/Log.h>
#include <gui/Surface.h>
#if ANDROID_MAJOR >= 11
#include <mediadrm/ICrypto.h>
#else
#include <media/ICrypto.h>
#endif
#include <media/stagefright/MediaDefs.h>
#include <media/stagefright/foundation/ALooper.h>
#include <media/stagefright/foundation/AMessage.h>
#include <media/stagefright/foundation/AUtils.h>
#include <media/stagefright/MediaBuffer.h>
#include <media/stagefright/MediaCodecList.h>
#include <media/stagefright/MediaCodec.h>
#include <media/stagefright/MetaData.h>
#include <media/stagefright/Utils.h>
#include <media/stagefright/SurfaceUtils.h>

using namespace android;

#if ANDROID_MAJOR > 7
#include <media/MediaCodecBuffer.h>
#else
#include <media/stagefright/foundation/ABuffer.h>
typedef ABuffer MediaCodecBuffer;
#endif

#define LOG_TAG "AsyncCodecSource"


AsyncCodecSource::SourceReader::SourceReader(AsyncCodecSource *codec,
                                             const sp<MediaSource> source)
    : Thread(false)
    , mRunning(true)
    , mCodec(codec)
    , mSource(source)
{
}

AsyncCodecSource::SourceReader::~SourceReader()
{
    stop();
}

void AsyncCodecSource::SourceReader::stop()
{
    {
        Mutex::Autolock lock(mInputIndices.lock);
        mRunning = false;
        mInputIndices.cond.signal();
    }
    // (returns at once when called from the thread itself, or when it never ran)
    requestExitAndWait();
}

size_t AsyncCodecSource::SourceReader::waitForInputBuffer()
{
    mInputIndices.lock.lock();

    // Checked before waiting too: a stop() that came first has already
    // signalled, and nothing would wake this wait again.
    while (mRunning && mInputIndices.buffers.empty()) {
        mInputIndices.cond.wait(mInputIndices.lock);
    }
    if (!mRunning) {
        mInputIndices.lock.unlock();
        return std::string::npos;
    }

    size_t index = *mInputIndices.buffers.begin();
    mInputIndices.buffers.erase(mInputIndices.buffers.begin());
    mInputIndices.lock.unlock();

    return index;
}

bool AsyncCodecSource::SourceReader::threadLoop(void)
{
    size_t index = waitForInputBuffer();
    if (index == std::string::npos) {
        return false;
    }

    DroidMediaBuffer *buffer = nullptr;
    mSource->read(&buffer);

    if (buffer) {
        mCodec->queueInputBuffer(buffer, index);
        buffer->release();
    } else {
        mCodec->queueEOS(index);
        Mutex::Autolock lock(mInputIndices.lock);
        mRunning = false;
    }
    Mutex::Autolock lock(mInputIndices.lock);
    return mRunning;
}

void AsyncCodecSource::SourceReader::inputBufferAvailable(size_t index)
{
    Mutex::Autolock lock(mInputIndices.lock);
    mInputIndices.buffers.push_back(index);
    mInputIndices.cond.signal();
}

//static
sp<MediaSource> AsyncCodecSource::Create(
        const sp<MediaSource> &source, const sp<AMessage> &srcFormat,
        bool isEncoder, uint32_t flags, const sp<ANativeWindow> &nativeWindow,
        const sp<ALooper> &looper, const char *desiredCodec, OMX_COLOR_FORMATTYPE colorFormat) {
    sp<Surface> surface = static_cast<Surface*>(nativeWindow.get());
    const char *mime = nullptr;
    sp<MetaData> meta = source->getFormat();
    CHECK(meta->findCString(kKeyMIMEType, &mime));

    sp<AMessage> format = new AMessage;
    if (srcFormat.get()) {
        // The unconfigured MediaSource comes from android::CameraSource.
        if (isEncoder && !strcmp(mime, "video/raw")) {
            // Use MediaCodecSource for the camera recorder.
            return MediaCodecSource::Create(looper, srcFormat, source, nullptr, flags);
        }
        format = srcFormat;
    } else if (convertMetaDataToMessage(meta, &format) != OK) {
        ALOGE("Cannot convertMetaDataToMessage()");
        return nullptr;
    }

    if (!isEncoder) {
#if ANDROID_MAJOR > 6
        format->setInt32("android._num-input-buffers", 12);
#else
        format->setInt32("inputbuffercnt", 12);
#endif
    }

    if (colorFormat != OMX_COLOR_FormatUnused) {
      format->setInt32("color-format", colorFormat);
    }

    Vector<AString> matchingCodecs;
    MediaCodecList::findMatchingCodecs(
            mime, isEncoder, flags, &matchingCodecs);

    for (const AString &componentName : matchingCodecs) {
        if (desiredCodec != nullptr && componentName.compare(desiredCodec)) {
            continue;
        }

        ALOGV("Attempting to allocate codec '%s'", componentName.c_str());
        sp<AsyncCodecSource> res = new AsyncCodecSource(componentName, source, looper,
                        strcmp(mime, MEDIA_MIMETYPE_AUDIO_VORBIS) == 0);

        if (res->mCodec != NULL) {
            ALOGI("Successfully allocated codec '%s'", componentName.c_str());
            if (res->configure(format, surface, isEncoder ? MediaCodec::CONFIGURE_FLAG_ENCODE : 0)) {
                if (surface != nullptr) {
#if ANDROID_MAJOR > 7
                    nativeWindowConnect(nativeWindow.get(), "AsyncCodecSource");
#else
                    native_window_api_connect(nativeWindow.get(),
                                                NATIVE_WINDOW_API_MEDIA);
#endif
                }
                return res;
            } else {
                ALOGE("Failed to configure codec '%s'", componentName.c_str());
            }
        }
        else {
            ALOGE("Failed to allocate codec '%s'", componentName.c_str());
        }
    }

    ALOGE("No matching codec! (mime: %s)", mime);
    return nullptr;
}

AsyncCodecSource::AsyncCodecSource(
        const AString &codecName, const sp<MediaSource> &source, const sp<ALooper> &looper,
        bool isVorbis)
    : mComponentName(codecName),
      mSource(source),
      mLooper(looper),
      mCodecLooper(new ALooper),
      mMeta(new MetaData),
      mUsingSurface(false),
      mIsVorbis(isVorbis) {
    mCodecLooper->setName("codec_looper");
    mCodecLooper->start(false, false, ANDROID_PRIORITY_AUDIO);

    mCodec = MediaCodec::CreateByComponentName(mCodecLooper, mComponentName);

    mReflector = new AHandlerReflector<AsyncCodecSource>(this);
    mLooper->registerHandler(mReflector);
    mNotify = new AMessage(0, mReflector);
    mSourceReader = new SourceReader(this, source);
}

AsyncCodecSource::~AsyncCodecSource() {
    // The reader thread calls into this source: it must end first, also when
    // stop() was never called. A started source is stopped for that, which
    // also wakes a reader waiting for a frame.
    bool started;
    {
        Mutexed<Output>::Locked me(mOutput);
        started = mState == STARTED || mState == ERROR;
    }
    if (started) {
        stop();
    }
    mSourceReader->stop();

    // Null when Create() could not allocate the codec (e.g. MediaCodec
    // without a ResourceManagerService refuses every codec).
    if (mCodec != nullptr) {
        mCodec->release();
    }
    mCodecLooper->stop();
    mLooper->unregisterHandler(mReflector->id());
}

bool AsyncCodecSource::configure(const sp<AMessage> format, const sp<Surface> surface, uint32_t flags) {
    mCodec->setCallback(mNotify);
    status_t err = mCodec->configure(format, surface, nullptr /* crypto */, flags);
    if (err != OK) {
        ALOGE("Failed to configure codec '%s'", mComponentName.c_str());
        return false;
    }
    mUsingSurface = surface != nullptr;

    // Set output format metadata
    sp<AMessage> outputFormat;
    mCodec->getOutputFormat(&outputFormat);
    sp<MetaData> meta = new MetaData;
    convertMessageToMetaData(outputFormat, meta);
    mMeta.lock().set(meta);

    mOutput.lock()->mReachedEOS = false;
    ALOGI("Configured codec '%s'!", mComponentName.c_str());
    return true;
}

status_t AsyncCodecSource::start(MetaData *params) {
    (void)params;
    Mutexed<Output>::Locked me(mOutput);

    if (mState != INIT) {
        return -EINVAL;
    }

    status_t res = mCodec->start();
    if (res == OK) {
        res = mSource->start();
    }

    if (res == OK) {
        res = mSourceReader->run("DroidMediaSourceReader");
    }

    if (res == OK) {
        mState = STARTED;
        me->mReachedEOS = false;
    } else {
        mState = ERROR;
    }
    return res;
}

status_t AsyncCodecSource::stop() {
    {
        Mutexed<Output>::Locked me(mOutput);
        if (mState != STARTED && mState != ERROR) {
            return -EINVAL;
        }

        // wait for any pending reads to complete
        mState = STOPPING;
        me->mAvailable.signal();
        while (me->mReading) {
            me.waitForCondition(me->mReadCondition);
        }
    }

    // Without mOutput's lock from here: the source's stop() waits for the
    // reader thread to hand back the frame it is queueing, and the reader
    // takes that lock when queueing fails (setError()). STOPPING keeps
    // read() out meanwhile.
    status_t res1 = mCodec->stop();
    if (res1 != OK) {
        mCodec->release();
    }
    // Ends a reader waiting for a frame (it then queues EOS)...
    status_t res2 = mSource->stop();
    // ...or for an input buffer, which a stopped codec never gives.
    mSourceReader->stop();

    Mutexed<Output>::Locked me(mOutput);
    if (res1 == OK && res2 == OK) {
        mState = STOPPED;
    } else {
        mState = ERROR;
    }
    return res1 != OK ? res1 : res2;
}

sp<MetaData> AsyncCodecSource::getFormat() {
    Mutexed<sp<MetaData>>::Locked meta(mMeta);
    return *meta;
}

AsyncCodecSource::Output::Output()
    : mReachedEOS(false),
      mReading(false) {
}

status_t AsyncCodecSource::read(
                    DroidMediaBuffer **buffer,
                    const ReadOptions *options) {
    *buffer = nullptr;
    Mutexed<Output>::Locked me(mOutput);
    status_t res = OK;

    // Only a read that goes on to wait counts as reading: stop() waits for
    // mReading to clear, and this early return would never clear it.
    if (mState != STARTED) {
        return ERROR_END_OF_STREAM;
    }
    me->mReading = true;

    // flush codec on seek
    MediaSource::ReadOptions::SeekMode mode;
    int64_t timeUs = 0ll;
    if (options != nullptr && options->getSeekTo(&timeUs, &mode)) {
        me->mReachedEOS = false;
        mCodec->flush();
    }

    *buffer = nullptr;
    while (me->mBufferQueue.size() == 0 && !me->mReachedEOS
           && mState == STARTED) {
        ALOGV("[%s] Waiting for output.", mComponentName.c_str());
        me.waitForCondition(me->mAvailable);
    }

    if (mState == ERROR) {
        // (not ERROR itself: that is a State, and read() returns a status_t)
        res = UNKNOWN_ERROR;
    }
    else if (!me->mBufferQueue.empty()) {
        *buffer = *me->mBufferQueue.begin();
        me->mBufferQueue.erase(me->mBufferQueue.begin());
        if (mOutputChanged && (*buffer)->size() == 0) {
            mOutputChanged = false;
            res = INFO_FORMAT_CHANGED;
        }
    }
    else if (me->mReachedEOS || mState != STARTED) {
        res = ERROR_END_OF_STREAM;
    }

    me->mReading = false;
    if (mState != STARTED) {
        me->mReadCondition.signal();
    }

    return res;
}

bool AsyncCodecSource::queueInputBuffer(DroidMediaBuffer *buffer, size_t index)
{
    int64_t timeUs = 0ll;
#if ANDROID_MAJOR >= 9
    CHECK(buffer->meta_data().findInt64(kKeyTime, &timeUs));
#else
    CHECK(buffer->meta_data()->findInt64(kKeyTime, &timeUs));
#endif

    sp<MediaCodecBuffer> inbuf;
    status_t err = mCodec->getInputBuffer(index, &inbuf);

    if (err != OK || inbuf == nullptr || inbuf->data() == nullptr) {
        ALOGE("[%s] Error: Input buffer #%zu invalid.", mComponentName.c_str(), index);
        setError();
        return false;
    }

    // Vorbis frames carry their sample count in 4 more bytes after the data.
    const size_t extra = mIsVorbis ? sizeof(int32_t) : 0;
    if (inbuf->capacity() < extra || buffer->range_length() > inbuf->capacity() - extra) {
        // Cut short, the frame would be queued with its full length all the
        // same, and the sample count written past the buffer.
        ALOGE("[%s] Error: frame of %zu bytes does not fit input buffer #%zu (%zu bytes)",
              mComponentName.c_str(), buffer->range_length(), index, inbuf->capacity());
        setError();
        return false;
    }
    size_t cpLen = buffer->range_length();
    memcpy(inbuf->base(), (uint8_t *)buffer->data() + buffer->range_offset(), cpLen);

    if (mIsVorbis) {
        int32_t numPageSamples;
        if (!
#if ANDROID_MAJOR >= 9
                buffer->meta_data().findInt32(kKeyValidSamples, &numPageSamples)
#else
                buffer->meta_data()->findInt32(kKeyValidSamples, &numPageSamples)
#endif
           ) {
            numPageSamples = -1;
        }
        memcpy(inbuf->base() + cpLen, &numPageSamples, sizeof(numPageSamples));
    }

    status_t res = mCodec->queueInputBuffer(
            index, 0 /* offset */, buffer->range_length() + (mIsVorbis ? 4 : 0),
            timeUs, 0 /* flags */);
    if (res != OK) {
        ALOGE("[%s] failed to queue input buffer #%zu", mComponentName.c_str(), index);
        setError();
    }
    ALOGV("[%s] Queued input buffer #%zu.", mComponentName.c_str(), index);
    return true;
}

void AsyncCodecSource::queueEOS(size_t index)
{
    mCodec->queueInputBuffer(index, 0, 0, 0, MediaCodec::BUFFER_FLAG_EOS);
    ALOGV("[%s] Queued EOS buffer #%zu.", mComponentName.c_str(), index);
}

void AsyncCodecSource::setError()
{
    Mutexed<Output>::Locked me(mOutput);
    if (mState == STARTED || mState == INIT) {
        mState = ERROR;
    }
    me->mAvailable.signal();
}

void AsyncCodecSource::flush()
{
    mCodec->flush();
}

void AsyncCodecSource::onMessageReceived(const sp<AMessage> &msg) {

    if (mCodec == nullptr) {
        return;
    }

    int32_t cbID;
    CHECK(msg->findInt32("callbackID", &cbID));
    if (cbID == MediaCodec::CB_INPUT_AVAILABLE) {
        int32_t index;
        CHECK(msg->findInt32("index", &index));
        ALOGV("[%s] Got input buffer #%d %d", mComponentName.c_str(), index, MediaCodec::CB_INPUT_AVAILABLE);
        mSourceReader->inputBufferAvailable(index);
    } else if (cbID == MediaCodec::CB_OUTPUT_FORMAT_CHANGED) {
        ALOGD("[%s] Output format changed! Buffers remaining: %zu", mComponentName.c_str(), mOutput.lock()->mBufferQueue.size());
        sp<AMessage> outputFormat;
        status_t err = mCodec->getOutputFormat(&outputFormat);
        if (err != OK) {
            ALOGE("[%s] Error fetching output format!", mComponentName.c_str());
            setError();
            return;
        }
        sp<MetaData> meta = new MetaData;
        convertMessageToMetaData(outputFormat, meta);
        mMeta.lock().set(meta);
        MediaBuffer     *buffer = new MediaBuffer(0);
        Mutexed<Output>::Locked me(mOutput);
        me->mBufferQueue.push_back(buffer);
        mOutputChanged = true;
        me->mAvailable.signal();
    } else if (cbID == MediaCodec::CB_OUTPUT_AVAILABLE) {
        int32_t index;
        size_t size;
        int64_t timeUs = 0ll;
        int32_t flags;

        CHECK(msg->findInt32("index", &index));
        CHECK(msg->findSize("size", &size));
        CHECK(msg->findInt64("timeUs", &timeUs));
        CHECK(msg->findInt32("flags", &flags));
        ALOGV("[%s] Got output buffer #%d", mComponentName.c_str(), index);
        if (flags & MediaCodec::BUFFER_FLAG_EOS) {
            mCodec->releaseOutputBuffer(index);
            Mutexed<Output>::Locked me(mOutput);
            me->mReachedEOS = true;
            me->mAvailable.signal();
            return;
        }

        sp<MediaCodecBuffer> out_buffer;
        status_t res = mCodec->getOutputBuffer(index, &out_buffer);
        if (res != OK) {
            ALOGE("[%s] could not get output buffer #%d",
                    mComponentName.c_str(), index);
            setError();
            mCodec->releaseOutputBuffer(index);
            return;
        }

        if (mUsingSurface && size > 0) {
            MediaBuffer     *buffer = new MediaBuffer(0);
            Mutexed<Output>::Locked me(mOutput);
            me->mBufferQueue.push_back(buffer);
            mCodec->renderOutputBufferAndRelease(index);
            me->mAvailable.signal();
        } else {
            MediaBuffer     *buffer = new MediaBuffer(size);
            CHECK_LE(out_buffer->size(), buffer->size());
            memcpy(buffer->data(), out_buffer->data(), out_buffer->size());
#if ANDROID_MAJOR >= 9
            buffer->meta_data().setInt64(kKeyTime, timeUs);
#else
            buffer->meta_data()->setInt64(kKeyTime, timeUs);
#endif
            Mutexed<Output>::Locked me(mOutput);
            me->mBufferQueue.push_back(buffer);
            mCodec->releaseOutputBuffer(index);
            me->mAvailable.signal();
        }
    } else if (cbID == MediaCodec::CB_ERROR) {
        status_t err;
        CHECK(msg->findInt32("err", &err));
        ALOGE("Codec (%s) reported error : %d",
                        mComponentName.c_str(), err);
        setError();
    } else {
        ALOGE("Codec (%s) unhandled callback id : %d",  mComponentName.c_str(), cbID);
    }
}

status_t AsyncCodecSource::setParameters(const sp<AMessage> &params)
{
    bool started;
    {
        Mutexed<Output>::Locked me(mOutput);
        started = mState == STARTED;
    }
    if (started && mCodec.get()) {
        return mCodec->setParameters(params);
    }
    return android::DEAD_OBJECT;
}
