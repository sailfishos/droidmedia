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

#undef LOG_TAG
#define LOG_TAG "DroidMediaBufferQueue"

#if ANDROID_MAJOR >= 6
static int slotIndex(const android::BufferItem &item) { return item.mSlot; }
#else
static int slotIndex(const android::BufferQueue::BufferItem &item) { return item.mBuf; }
#endif

DroidMediaBufferQueueListener::DroidMediaBufferQueueListener(_DroidMediaBufferQueue *queue) :
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR == 4) || ANDROID_MAJOR >= 5
  ProxyConsumerListener(NULL),
#endif
  m_queue(queue)
{
}

DroidMediaBufferQueueListener::~DroidMediaBufferQueueListener()
{
}

void DroidMediaBufferQueueListener::onFrameAvailable()
{
  android::sp<_DroidMediaBufferQueue> queue(m_queue.promote());
  if (queue.get()) {
    queue->frameAvailable();
  }
}

void DroidMediaBufferQueueListener::onBuffersReleased()
{
  android::sp<_DroidMediaBufferQueue> queue(m_queue.promote());
  if (queue.get()) {
    queue->buffersReleased();
  }
}

_DroidMediaBufferQueue::_DroidMediaBufferQueue(const char *name) :
  m_listener(new DroidMediaBufferQueueListener(this)),
  m_data(0) {

  memset(&m_cb, 0x0, sizeof(m_cb));
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

void _DroidMediaBufferQueue::frameAvailable() {
  DroidMediaBufferItem item;

#if (ANDROID_MAJOR == 4 && ANDROID_MINOR < 4)
  int err = m_queue->acquireBuffer(&item);
#else
  int err = m_queue->acquireBuffer(&item, 0);
#endif

  if (err != android::OK) {
    // Invalid operation means we've tried to acquire too many concurrent buffers. That's expected,
    // we'll attempt to over acquire a few times at the start and then an equilibrium will be
    // reached.
    if (err != android::INVALID_OPERATION) {
        ALOGE("Failed to acquire item from the queue. Error 0x%x", -err);
    }
    return;
  }

  m_lock.lock();

  DroidMediaBufferSlot &slot = m_slots[slotIndex(item)];

  if (item.mGraphicBuffer != NULL) {
    static_cast<DroidMediaBufferItem &>(slot) = item;

    slot.droidBuffer = new DroidMediaBuffer(slot, this);

    // Keep the original reference count for ourselves and give one to the buffer_created
    // callback to release with droid_media_buffer_destroy. If there is no buffer_created
    // callback or it returns false we dereference both.
    slot.droidBuffer->incStrong(0);

    if (!m_data || !m_cb.buffer_created(m_data, slot.droidBuffer.get())) {
      slot.droidBuffer->decStrong(0);
      slot.droidBuffer.clear();
    }
  } else {
    slot.mTransform = item.mTransform;
    slot.mScalingMode = item.mScalingMode;
    slot.mTimestamp = item.mTimestamp;
    slot.mFrameNumber = item.mFrameNumber;
    slot.mCrop = item.mCrop;

    if (slot.droidBuffer.get()) {
      slot.droidBuffer->update(slot);

      // Recreate the user data if it was cleared for some reason like the buffer pool resetting
      // itself without the queue being recreated. In reality buffer pools may always outlive
      // queue so may never trigger.
      if (!slot.droidBuffer->m_userData && m_data) {
        slot.droidBuffer->incStrong(0);
        if (!m_cb.buffer_created(m_data, slot.droidBuffer.get())) {
            slot.droidBuffer->decStrong(0);
            slot.droidBuffer.clear();
        }
      }
    }
  }

  if (slot.droidBuffer.get() && m_data && m_cb.frame_available(m_data, slot.droidBuffer.get())) {
    m_lock.unlock();
  } else {
    m_lock.unlock();

    ALOGE("Client wasn't able to handle a received frame.");

    releaseMediaBuffer(slotIndex(slot), 0, 0);
  }
}

void _DroidMediaBufferQueue::buffersReleased() {
  for (int i = 0; i < android::BufferQueue::NUM_BUFFER_SLOTS; ++i) {
    DroidMediaBufferSlot &slot = m_slots[i];
    slot.droidBuffer.clear();
    slot.mGraphicBuffer = 0;
  }

  android::AutoMutex locker(&m_lock);

  if (m_data) {
    m_cb.buffers_released(m_data);
  }
}

int _DroidMediaBufferQueue::releaseMediaBuffer(int index, EGLDisplay dpy, EGLSyncKHR fence) {

    int err = m_queue->releaseBuffer(index,
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR == 4) || ANDROID_MAJOR >= 5
    // TODO: fix this when we do video rendering
    m_slots[index].mFrameNumber,
#endif
    dpy, fence
#if (ANDROID_MAJOR == 4 && (ANDROID_MINOR >= 2)) || ANDROID_MAJOR >= 5
                                             // TODO: fix this when we do video rendering
    , android::Fence::NO_FENCE
#endif
                                             );
  if (err != android::NO_ERROR) {
    ALOGE("error releasing item. Error 0x%x", -err);
  }

  return err;
}

#if ANDROID_MAJOR < 5
static const int staleBuffer = android::BufferQueue::STALE_BUFFER_SLOT;
#else
static const int staleBuffer = android::IGraphicBufferConsumer::STALE_BUFFER_SLOT;
#endif

void _DroidMediaBufferQueue::releaseMediaBuffer(DroidMediaBuffer *buffer,
					       EGLDisplay dpy, EGLSyncKHR fence) {

  int err = releaseMediaBuffer(buffer->m_slot, dpy, fence);

  switch (err) {
  case android::NO_ERROR:
      break;

  case staleBuffer:
      ALOGW("Released stale buffer %d", buffer->m_slot);
      break;

  default:
      ALOGE("Error 0x%x releasing buffer %d", -err, buffer->m_slot);
      break;
  }
}

void _DroidMediaBufferQueue::setCallbacks(DroidMediaBufferQueueCallbacks *cb, void *data) {
  android::AutoMutex locker(&m_lock);

  if (!cb) {
    memset(&m_cb, 0x0, sizeof(m_cb));
  } else {
    memcpy(&m_cb, cb, sizeof(m_cb));
  }

  m_data = data;
}

void droid_media_buffer_queue_set_callbacks(DroidMediaBufferQueue *queue,
    DroidMediaBufferQueueCallbacks *cb, void *data) {

  if (queue) {
    queue->setCallbacks(cb, data);
  }
}

int droid_media_buffer_queue_length() {
  return android::BufferQueue::NUM_BUFFER_SLOTS;
}
