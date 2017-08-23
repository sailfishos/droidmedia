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

#include "private.h"
#include "droidmediabuffer.h"
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR < 4)
#include <gui/SurfaceTextureClient.h>
#else
#include <gui/Surface.h>
#endif

DroidMediaBufferQueueListener::DroidMediaBufferQueueListener() :
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR == 4) || ANDROID_MAJOR >= 5
  ProxyConsumerListener(NULL),
#endif
  m_data(0)
{
  memset(&m_cb, 0x0, sizeof(m_cb));
}

void DroidMediaBufferQueueListener::onFrameAvailable()
{
  m_lock.lock();

  if (m_cb.frame_available) {
    m_cb.frame_available(m_data);
  }

  m_lock.unlock();
}

void DroidMediaBufferQueueListener::onBuffersReleased()
{
  m_lock.lock();

  if (m_cb.buffers_released) {
    m_cb.buffers_released(m_data);
  }

  m_lock.unlock();
}

void DroidMediaBufferQueueListener::setCallbacks(DroidMediaBufferQueueCallbacks *cb, void *data) {
  m_lock.lock();

  if (!cb) {
    memset(&m_cb, 0x0, sizeof(m_cb));
  } else {
    memcpy(&m_cb, cb, sizeof(m_cb));
  }

  m_data = data;

  m_lock.unlock();
}

_DroidMediaBufferQueue::_DroidMediaBufferQueue(const char *name) {
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR < 2)
  m_queue = new android::BufferQueue(true, android::BufferQueue::MIN_UNDEQUEUED_BUFFERS);
#elif ANDROID_MAJOR < 5
  m_queue = new android::BufferQueue();
#else
  android::BufferQueue::createBufferQueue(&m_producer, &m_queue);
#endif

#if (ANDROID_MAJOR == 4 && ANDROID_MINOR < 4)
  m_queue->setSynchronousMode(false);
#else
  // We need to acquire up to 2 buffers
  // One is being rendered and the other one is waiting to be rendered.
  m_queue->setMaxAcquiredBufferCount(2);
#endif

  m_queue->setConsumerName(android::String8(name));
  m_queue->setConsumerUsageBits(android::GraphicBuffer::USAGE_HW_TEXTURE);

  m_listener = new DroidMediaBufferQueueListener;
}

_DroidMediaBufferQueue::~_DroidMediaBufferQueue()
{
  disconnectListener();
  m_listener.clear();
}

bool _DroidMediaBufferQueue::connectListener()
{
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR < 4)
  if (m_queue->consumerConnect(m_listener) != android::NO_ERROR) {
#else
  if (m_queue->consumerConnect(m_listener, false) != android::NO_ERROR) {
#endif
    ALOGE("Failed to set buffer consumer");

    return false;
  }

  return true;
}

void _DroidMediaBufferQueue::disconnectListener()
{
  m_queue->consumerDisconnect();
}

void _DroidMediaBufferQueue::attachToCamera(android::sp<android::Camera>& camera) {
#if ANDROID_MAJOR == 4 && ANDROID_MINOR < 4
    camera->setPreviewTexture(m_queue);
#elif ANDROID_MAJOR < 5
    camera->setPreviewTarget(m_queue);
#else
    camera->setPreviewTarget(m_producer);
#endif
}

ANativeWindow *_DroidMediaBufferQueue::window() {
#if ANDROID_MAJOR == 4 && ANDROID_MINOR < 4
    android::sp<android::ISurfaceTexture> texture = m_queue;
    return new android::SurfaceTextureClient(texture);
#elif ANDROID_MAJOR < 5
    android::sp<android::IGraphicBufferProducer> texture = m_queue;
    return new android::Surface(texture, true);
#else
    android::sp<android::IGraphicBufferProducer> texture = m_producer;
    return new android::Surface(texture, true);
#endif
}

DroidMediaBuffer *_DroidMediaBufferQueue::acquireMediaBuffer(DroidMediaBufferCallbacks *cb)
{
#if ANDROID_MAJOR < 6
  android::BufferQueue::BufferItem buffer;
#else
  android::BufferItem buffer;
#endif
  int num;

#if (ANDROID_MAJOR == 4 && ANDROID_MINOR < 4)
  int err = m_queue->acquireBuffer(&buffer);
#else
  int err = m_queue->acquireBuffer(&buffer, 0);
#endif

  if (err != android::OK) {
    ALOGE("DroidMediaBufferQueue: Failed to acquire buffer from the queue. Error 0x%x", -err);
    return NULL;
  }

  // TODO: Here we are working around the fact that BufferQueue will send us an mGraphicBuffer
  // only when it changes. We can integrate SurfaceTexture but thart needs a lot of
  // change in the whole stack
#if ANDROID_MAJOR >= 6
  num = buffer.mSlot;
#else
  num = buffer.mBuf;
#endif

  if (buffer.mGraphicBuffer != NULL) {
    m_slots[num] = buffer;
  } else {
    m_slots[num].mTransform = buffer.mTransform;
    m_slots[num].mScalingMode = buffer.mScalingMode;
    m_slots[num].mTimestamp = buffer.mTimestamp;
    m_slots[num].mFrameNumber = buffer.mFrameNumber;
    m_slots[num].mCrop = buffer.mCrop;
  }

  if (m_slots[num].mGraphicBuffer == NULL) {
    ALOGE("DroidMediaBufferQueue: Got a buffer without real data");

    DroidMediaBuffer *buffer = new DroidMediaBuffer(m_slots[num], this, NULL, NULL, NULL);
    err = releaseMediaBuffer(buffer, EGL_NO_DISPLAY, EGL_NO_SYNC_KHR);

    if (err != android::NO_ERROR) {
      ALOGE("DroidMediaBufferQueue: error releasing buffer. Error 0x%x", -err);
    }

    return NULL;
  }

  return new DroidMediaBuffer(m_slots[num], this,
                              cb ? cb->data : NULL,
                              cb ? cb->ref : NULL,
                              cb ? cb->unref : NULL);
}

int _DroidMediaBufferQueue::releaseMediaBuffer(DroidMediaBuffer *buffer,
					       EGLDisplay dpy, EGLSyncKHR fence) {

    int err = m_queue->releaseBuffer(buffer->m_slot,
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR == 4) || ANDROID_MAJOR >= 5
    // TODO: fix this when we do video rendering
    buffer->m_frameNumber,
#endif
    dpy, fence
#if (ANDROID_MAJOR == 4 && (ANDROID_MINOR >= 2)) || ANDROID_MAJOR >= 5
					     // TODO: fix this when we do video rendering
    , android::Fence::NO_FENCE
#endif
					     );

    return err;
}

void _DroidMediaBufferQueue::setCallbacks(DroidMediaBufferQueueCallbacks *cb, void *data) {
  assert(m_listener.get());
  m_listener->setCallbacks(cb, data);
}

bool _DroidMediaBufferQueue::acquireAndRelease(DroidMediaBufferInfo *info) {
  DroidMediaBuffer *buff = acquireMediaBuffer(NULL);

  if (buff) {
    if (info) {
      droid_media_buffer_get_info (buff, info);
    }

    droid_media_buffer_release(buff, EGL_NO_DISPLAY, EGL_NO_SYNC_KHR);
    return true;
  }

  return false;
}

extern "C" {
DroidMediaBuffer *droid_media_buffer_queue_acquire_buffer(DroidMediaBufferQueue *queue,
    DroidMediaBufferCallbacks *cb)
{
  return queue->acquireMediaBuffer(cb);
}

void droid_media_buffer_queue_set_callbacks(DroidMediaBufferQueue *queue,
    DroidMediaBufferQueueCallbacks *cb, void *data)
{

  return queue->setCallbacks(cb, data);
}

bool droid_media_buffer_queue_acquire_and_release(DroidMediaBufferQueue *queue, DroidMediaBufferInfo *info)
{
  return queue->acquireAndRelease(info);
}

};
