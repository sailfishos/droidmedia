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

DroidMediaBufferQueueListener::DroidMediaBufferQueueListener() :
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4
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

DroidMediaBufferQueue::DroidMediaBufferQueue(const char *name) :
#if ANDROID_MAJOR == 4 && (ANDROID_MINOR == 4 || ANDROID_MINOR == 2)
  android::BufferQueue()
#else
  android::BufferQueue(true, android::BufferQueue::MIN_UNDEQUEUED_BUFFERS)
#endif
{
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4
  // We need to acquire up to 2 buffers
  // One is being rendered and the other one is waiting to be rendered.
  setMaxAcquiredBufferCount(2);
#else
  setSynchronousMode(false);
#endif

  setConsumerName(android::String8(name));
  setConsumerUsageBits(android::GraphicBuffer::USAGE_HW_TEXTURE);
}

DroidMediaBufferQueue::~DroidMediaBufferQueue()
{
  m_listener.clear();
}

bool DroidMediaBufferQueue::connectListener()
{
  assert(m_listener.get() == NULL);

  m_listener = new DroidMediaBufferQueueListener;

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4
  if (consumerConnect(m_listener, false) != android::NO_ERROR) {
#else
  if (consumerConnect(m_listener) != android::NO_ERROR) {
#endif
    ALOGE("Failed to set buffer consumer");

    m_listener.clear();

    return false;
  }

  return true;
}

DroidMediaBuffer *DroidMediaBufferQueue::acquireMediaBuffer(DroidMediaBufferCallbacks *cb)
{
  android::BufferQueue::BufferItem buffer;
  int num;
  int err = acquireBuffer(&buffer
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4 // TODO: UGLY!
    , 0
#endif
    );

  if (err != android::OK) {
    ALOGE("DroidMediaBufferQueue: Failed to acquire buffer from the queue. Error 0x%x", -err);
    return NULL;
  }

  // TODO: Here we are working around the fact that BufferQueue will send us an mGraphicBuffer
  // only when it changes. We can integrate SurfaceTexture but thart needs a lot of
  // change in the whole stack
  num = buffer.mBuf;

  if (buffer.mGraphicBuffer != NULL) {
    m_slots[num] = buffer;
  }

  if (m_slots[num].mGraphicBuffer == NULL) {
    int err;
    ALOGE("DroidMediaBufferQueue: Got a buffer without real data");
    err = releaseBuffer(buffer.mBuf,
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4 // TODO: fix this when we do video rendering
    buffer.mFrameNumber,
#endif
    EGL_NO_DISPLAY, EGL_NO_SYNC_KHR
#if ANDROID_MAJOR == 4 && (ANDROID_MINOR == 4 || ANDROID_MINOR == 2) // TODO: fix this when we do video rendering
    , android::Fence::NO_FENCE
#endif
    );

    if (err != android::NO_ERROR) {
      ALOGE("DroidMediaBufferQueue: error releasing buffer. Error 0x%x", -err);
    }

    return NULL;
  }

  m_slots[num].mTransform = buffer.mTransform;
  m_slots[num].mScalingMode = buffer.mScalingMode;
  m_slots[num].mTimestamp = buffer.mTimestamp;
  m_slots[num].mFrameNumber = buffer.mFrameNumber;
  m_slots[num].mCrop = buffer.mCrop;

  return new DroidMediaBuffer(m_slots[num], this, cb->data, cb->ref, cb->unref);
}

void DroidMediaBufferQueue::setCallbacks(DroidMediaBufferQueueCallbacks *cb, void *data) {
  assert(m_listener.get());
  m_listener->setCallbacks(cb, data);
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
};
