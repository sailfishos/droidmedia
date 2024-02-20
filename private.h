/*
 * Copyright (C) 2014-2015 Jolla Ltd.
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

#ifndef DROID_MEDIA_PRIVATE_H
#define DROID_MEDIA_PRIVATE_H

#include "droidmedia.h"
#include <gui/BufferQueue.h>
#include <camera/Camera.h>
#if ANDROID_MAJOR >= 9 && ANDROID_MAJOR <= 10
#    include <media/MediaSource.h>
#else
#    include <media/stagefright/MediaSource.h>
#endif
#include "droidmediabuffer.h"
#include "droidmediacamera.h"
#include "droidmediacodec.h"
#if ANDROID_MAJOR >= 5
#    include <media/stagefright/foundation/ALooper.h>
#endif

struct _DroidMediaBufferQueue;

class DroidMediaBufferQueueListener :
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR < 4)
    public android::BufferQueue::ConsumerListener
{
#else
    public android::BufferQueue::ProxyConsumerListener
{
#endif
public:
    DroidMediaBufferQueueListener(_DroidMediaBufferQueue *queue);
    ~DroidMediaBufferQueueListener();

    void onFrameAvailable();
    void onBuffersReleased();

#if ANDROID_MAJOR >= 5
    void onFrameAvailable(const android::BufferItem &) { onFrameAvailable(); }
    void onSidebandStreamChanged() { }
#endif

private:
    android::wp<_DroidMediaBufferQueue> m_queue;
};

class DroidMediaBufferSlot : public DroidMediaBufferItem
{
public:
    android::sp<DroidMediaBuffer> droidBuffer;
};

struct _DroidMediaBufferQueue : public android::RefBase
{
public:
    _DroidMediaBufferQueue(const char *name);
    ~_DroidMediaBufferQueue();

    bool connectListener();
    void disconnectListener();

    void attachToCameraPreview(android::sp<android::Camera> &camera);
    void attachToCameraVideo(android::sp<android::Camera> &camera);
    ANativeWindow *window();

    void releaseMediaBuffer(DroidMediaBuffer *buffer, EGLDisplay dpy, EGLSyncKHR fence);

    void setCallbacks(DroidMediaBufferQueueCallbacks *cb, void *data);

    void buffersReleased();

private:
    friend class DroidMediaBufferQueueListener;

    void frameAvailable();

    int releaseMediaBuffer(int index, EGLDisplay dpy, EGLSyncKHR fence);

#if ANDROID_MAJOR >= 5
    android::sp<android::IGraphicBufferProducer> m_producer;
    android::sp<android::IGraphicBufferConsumer> m_queue;
#else
    android::sp<android::BufferQueue> m_queue;
#endif

    DroidMediaBufferSlot m_slots[android::BufferQueue::NUM_BUFFER_SLOTS];

    android::sp<DroidMediaBufferQueueListener> m_listener;
    android::Mutex m_lock;

    DroidMediaBufferQueueCallbacks m_cb;
    void *m_data;
};

android::sp<android::Camera> droid_media_camera_get_camera(DroidMediaCamera *camera);
android::sp<android::MediaSource>
droid_media_codec_create_encoder_raw(DroidMediaCodecEncoderMetaData *meta,
#if ANDROID_MAJOR >= 5
                                     android::sp<android::ALooper> looper,
#endif
                                     android::sp<android::MediaSource> src);

#endif /* DROID_MEDIA_PRIVATE_H */
