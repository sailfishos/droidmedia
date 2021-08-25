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

#include "AsyncDecodingSource.h"
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

//#define LOG_NDEBUG 0
#define LOG_TAG "AsyncDecodingSource"



//static
sp<AsyncDecodingSource> AsyncDecodingSource::Create(
        const sp<MediaSource> &source, uint32_t flags, const sp<ANativeWindow> &nativeWindow,
        const sp<ALooper> &looper, const char *desiredCodec) {
    sp<Surface> surface = static_cast<Surface*>(nativeWindow.get());
    const char *mime = nullptr;
    sp<MetaData> meta = source->getFormat();
    CHECK(meta->findCString(kKeyMIMEType, &mime));

    sp<AMessage> format = new AMessage;
    if (convertMetaDataToMessage(meta, &format) != OK) {
        return nullptr;
    }

    Vector<AString> matchingCodecs;
    MediaCodecList::findMatchingCodecs(
            mime, false /* encoder */, flags, &matchingCodecs);

    for (const AString &componentName : matchingCodecs) {
        if (desiredCodec != nullptr && componentName.compare(desiredCodec)) {
            continue;
        }

        ALOGV("Attempting to allocate codec '%s'", componentName.c_str());
        sp<AsyncDecodingSource> res = new AsyncDecodingSource(componentName, source, looper,
                        strcmp(mime, MEDIA_MIMETYPE_AUDIO_VORBIS) == 0);

        if (res->mCodec != NULL) {
            ALOGI("Successfully allocated codec '%s'", componentName.c_str());
            if (res->configure(format, surface, 0)) {
                if (surface != nullptr) {
#if ANDROID_MAJOR > 7
                    nativeWindowConnect(nativeWindow.get(), "AsyncDecodingSource");
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

    ALOGE("No matching decoder! (mime: %s)", mime);
    return nullptr;
}

AsyncDecodingSource::AsyncDecodingSource(
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

    mReflector = new AHandlerReflector<AsyncDecodingSource>(this);
    mLooper->registerHandler(mReflector);
    mNotify = new AMessage(0, mReflector);
}

AsyncDecodingSource::~AsyncDecodingSource() {
    mCodec->release();
    mCodecLooper->stop();
    mLooper->unregisterHandler(mReflector->id());
}

bool AsyncDecodingSource::configure(const sp<AMessage> format, const sp<Surface> surface, uint32_t flags) {
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

status_t AsyncDecodingSource::start(MetaData *params) {
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
        mState = STARTED;
        me->mReachedEOS = false;
    } else {
        mState = ERROR;
    }
    return res;
}

status_t AsyncDecodingSource::stop() {
    Mutexed<Output>::Locked me(mOutput);
    if (mState != STARTED) {
        return -EINVAL;
    }

    // wait for any pending reads to complete
    mState = STOPPING;
    while (me->mReading) {
        me.waitForCondition(me->mReadCondition);
    }

    status_t res1 = mCodec->stop();
    if (res1 != OK) {
        mCodec->release();
    }
    status_t res2 = mSource->stop();
    if (res1 == OK && res2 == OK) {
        mState = STOPPED;
    } else {
        mState = ERROR;
    }
    return res1 != OK ? res1 : res2;
}

sp<MetaData> AsyncDecodingSource::getFormat() {
    Mutexed<sp<MetaData>>::Locked meta(mMeta);
    return *meta;
}

AsyncDecodingSource::Output::Output()
    : mReachedEOS(false),
      mReading(false) {
}

status_t AsyncDecodingSource::read(
                    DroidMediaBuffer **buffer,
                    const ReadOptions *options) {
    *buffer = nullptr;
    Mutexed<Output>::Locked me(mOutput);
    me->mReading = true;
    status_t res = OK;

    if (mState != STARTED) {
        return ERROR_END_OF_STREAM;
    }

    // flush codec on seek
    MediaSource::ReadOptions::SeekMode mode;
    int64_t timeUs = 0ll;
    if (options != nullptr && options->getSeekTo(&timeUs, &mode)) {
        me->mReachedEOS = false;
        mCodec->flush();
    }

    *buffer = nullptr;
    while (me->mBufferQueue.size() == 0 && !me->mReachedEOS) {
        ALOGV("[%s] Waiting for output.", mComponentName.c_str());
        me.waitForCondition(me->mAvailable);
    }

    if (mState == ERROR) {
        res = ERROR;
    }
    else if (!me->mBufferQueue.empty()) {
        *buffer = *me->mBufferQueue.begin();
        me->mBufferQueue.erase(me->mBufferQueue.begin());
        if (mOutputChanged && (*buffer)->size() == 0) {
            mOutputChanged = false;
            res = INFO_FORMAT_CHANGED;
        }
    }
    else if (me->mReachedEOS) {
        res = ERROR_END_OF_STREAM;
    }

    me->mReading = false;
    if (mState != STARTED) {
        me->mReadCondition.signal();
    }

    return res;
}

void AsyncDecodingSource::onMessageReceived(const sp<AMessage> &msg) {

    if (mCodec == nullptr) {
        return;
    }

    int32_t cbID;
    CHECK(msg->findInt32("callbackID", &cbID));
    if (cbID == MediaCodec::CB_INPUT_AVAILABLE) {
        int32_t index;
        CHECK(msg->findInt32("index", &index));
        ALOGV("[%s] Got input buffer #%d", mComponentName.c_str(), index);
        mAvailInputIndices.push_back(index);
        if (!mFeeding) {
            while (!mAvailInputIndices.empty()) {
                DroidMediaBuffer *srcbuf =  nullptr;
                mFeeding = true;
                size_t bufferIndex = *mAvailInputIndices.begin();
                mAvailInputIndices.erase(mAvailInputIndices.begin());
                mSource->read(&srcbuf);
                uint32_t flags = 0;
                int64_t timeUs = 0ll;

                if (srcbuf != nullptr) {
#if ANDROID_MAJOR >= 9
                    CHECK(srcbuf->meta_data().findInt64(kKeyTime, &timeUs));
#else
                    CHECK(srcbuf->meta_data()->findInt64(kKeyTime, &timeUs));
#endif

                    sp<MediaCodecBuffer> inbuf;
                    status_t err = mCodec->getInputBuffer(bufferIndex, &inbuf);

                    if (err != OK || inbuf == nullptr || inbuf->data() == nullptr) {
                        ALOGE("[%s] Error: Input buffer #%d invalid.", mComponentName.c_str(), index);
                        srcbuf->release();
                        mState = ERROR;
                        return;
                    }

                    size_t cpLen = min(srcbuf->range_length(), inbuf->capacity());
                    memcpy(inbuf->base(), (uint8_t *)srcbuf->data() + srcbuf->range_offset(),
                                    cpLen );

                    if (mIsVorbis) {
                        int32_t numPageSamples;
                        if (!
#if ANDROID_MAJOR >= 9
                                        srcbuf->meta_data().findInt32(kKeyValidSamples, &numPageSamples)
#else
                                        srcbuf->meta_data()->findInt32(kKeyValidSamples, &numPageSamples)
#endif
                    ) {
                            numPageSamples = -1;
                        }
                        memcpy(inbuf->base() + cpLen, &numPageSamples, sizeof(numPageSamples));
                    }

                    status_t res = mCodec->queueInputBuffer(
                                    bufferIndex, 0 /* offset */, srcbuf->range_length() + (mIsVorbis ? 4 : 0),
                                    timeUs, 0 /* flags */);
                    if (res != OK) {
                        ALOGE("[%s] failed to queue input buffer #%d", mComponentName.c_str(), index);
                        mState = ERROR;
                    }
                    ALOGV("[%s] Queued input buffer #%d.", mComponentName.c_str(), index);
                    srcbuf->release();
                } else {
                    flags = MediaCodec::BUFFER_FLAG_EOS;
                    mCodec->queueInputBuffer(bufferIndex, 0, 0, timeUs, flags);
                }
            }
        mFeeding = false;
        }
    } else if (cbID == MediaCodec::CB_OUTPUT_FORMAT_CHANGED) {
        ALOGD("[%s] Output format changed! Buffers remaining: %zu", mComponentName.c_str(), mOutput.lock()->mBufferQueue.size());
        sp<AMessage> outputFormat;
        status_t err = mCodec->getOutputFormat(&outputFormat);
        if (err != OK) {
            ALOGE("[%s] Error fetching output format!", mComponentName.c_str());
            mState = ERROR;
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
            mOutput.lock()->mReachedEOS = true;
            mOutput.lock()->mAvailable.signal();
            return;
        }

        sp<MediaCodecBuffer> out_buffer;
        status_t res = mCodec->getOutputBuffer(index, &out_buffer);
        if (res != OK) {
            ALOGE("[%s] could not get output buffer #%d",
                    mComponentName.c_str(), index);
            mState = ERROR;
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
        ALOGE("Decoder (%s) reported error : 0x%d",
                        mComponentName.c_str(), err);
        mState = ERROR;
   } else {
       ALOGE("Decoder (%s) unhandled callback id : 0x%d",  mComponentName.c_str(), cbID);
   }
}
