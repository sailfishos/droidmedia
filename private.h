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

#ifndef DROID_MEDIA_PRIVATE_H
#define DROID_MEDIA_PRIVATE_H

#include "droidmedia.h"
#include <gui/BufferQueue.h>

class BufferQueueListener : public android::BufferQueue::ConsumerListener {
public:
  BufferQueueListener();
  void onFrameAvailable();
  void onBuffersReleased();
  void setCallbacks(DroidMediaRenderingCallbacks *cb, void *data);

private:
    DroidMediaRenderingCallbacks m_cb;
    void *m_data;
};

android::sp<android::BufferQueue> createBufferQueue(const char *name,
			            android::sp<BufferQueueListener>& listener);

#endif /* DROID_MEDIA_PRIVATE_H */
