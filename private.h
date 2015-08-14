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

class DroidMediaBufferQueueListener :
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR == 4) || ANDROID_MAJOR == 5
public android::BufferQueue::ProxyConsumerListener {
#else
public android::BufferQueue::ConsumerListener {
#endif
public:
  DroidMediaBufferQueueListener();
  void onFrameAvailable();
  void onBuffersReleased();
  void setCallbacks(DroidMediaBufferQueueCallbacks *cb, void *data);

#if ANDROID_MAJOR == 5
  void onFrameAvailable(const android::BufferItem&) { onFrameAvailable(); }
  void onSidebandStreamChanged() {}
#endif

private:
  DroidMediaBufferQueueCallbacks m_cb;
  void *m_data;

  android::Mutex m_lock;
};

struct _DroidMediaBufferQueue : public android::RefBase {
public:
  _DroidMediaBufferQueue(const char *name);
  ~_DroidMediaBufferQueue();

  bool connectListener();
  void disconnectListener();

  void attachToCamera(android::sp<android::Camera>& camera);
  ANativeWindow *window();

  int releaseMediaBuffer(DroidMediaBuffer *buffer, EGLDisplay dpy, EGLSyncKHR fence);

  DroidMediaBuffer *acquireMediaBuffer(DroidMediaBufferCallbacks *cb);

  void setCallbacks(DroidMediaBufferQueueCallbacks *cb, void *data);

  bool acquireAndRelease(DroidMediaBufferInfo *info);

private:
#if ANDROID_MAJOR == 5
  android::sp<android::IGraphicBufferProducer> m_producer;
  android::sp<android::IGraphicBufferConsumer> m_queue;
#else
  android::sp<android::BufferQueue> m_queue;
#endif
  android::BufferQueue::BufferItem m_slots[android::BufferQueue::NUM_BUFFER_SLOTS];

  android::sp<DroidMediaBufferQueueListener> m_listener;
};

#endif /* DROID_MEDIA_PRIVATE_H */
