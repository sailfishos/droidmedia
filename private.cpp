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

BufferQueueListener::BufferQueueListener() :
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4
  ProxyConsumerListener(NULL),
#endif
    m_data(0)
{
    memset(&m_cb, 0x0, sizeof(m_cb));
}

void BufferQueueListener::onFrameAvailable()
{
    if (m_cb.frame_available) {
        m_cb.frame_available(m_data);
    }
}

void BufferQueueListener::onBuffersReleased()
{
    if (m_cb.buffers_released) {
        m_cb.buffers_released(m_data);
    }
}

void BufferQueueListener::setCallbacks(DroidMediaRenderingCallbacks *cb, void *data) {
    memcpy(&m_cb, cb, sizeof(m_cb));
    m_data = data;
}

android::sp<android::BufferQueue> createBufferQueue(const char *name,
						    android::sp<BufferQueueListener>& listener)
{
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4
  android::BufferQueue *queue = new android::BufferQueue;

  // TODO: This number is arbitrary but if we don't do that then playback gets stuck. I need to debug that
  // and get rid of this hack
  queue->setMaxAcquiredBufferCount(6);
#else
  android::BufferQueue *queue =
    new android::BufferQueue(true, android::BufferQueue::MIN_UNDEQUEUED_BUFFERS);
  queue->setSynchronousMode(false);
#endif

  queue->setConsumerName(android::String8(name));
  queue->setConsumerUsageBits(android::GraphicBuffer::USAGE_HW_TEXTURE);

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4
  if (queue->consumerConnect(listener, true) != android::NO_ERROR) {
#else
  if (queue->consumerConnect(listener) != android::NO_ERROR) {
#endif
    ALOGE("Failed to set buffer consumer");
    delete queue;
    return NULL;
  }

  return queue;
}
