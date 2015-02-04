/*
 * Copyright (C) 2014 Jolla Ltd.
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
  android::BufferQueue *
    queue(new android::BufferQueue(true, android::BufferQueue::MIN_UNDEQUEUED_BUFFERS));

  queue->setConsumerName(android::String8(name));
  queue->setConsumerUsageBits(android::GraphicBuffer::USAGE_HW_TEXTURE);
  queue->setSynchronousMode(false);

  if (queue->consumerConnect(listener) != android::NO_ERROR) {
    ALOGE("Failed to set buffer consumer");
    delete queue;
    return NULL;
  }

  return queue;
}
