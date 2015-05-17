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

#ifndef DROID_MEDIA_BUFFER_H
#define DROID_MEDIA_BUFFER_H

#include <system/window.h>
#include <gui/BufferQueue.h>
#include "droidmedia.h"

struct _DroidMediaBuffer : public ANativeWindowBuffer
{
public:
  _DroidMediaBuffer(android::BufferQueue::BufferItem& buffer,
		   android::sp<DroidMediaBufferQueue> queue,
		   void *data,
		   DroidMediaCallback ref,
		   DroidMediaCallback unref);

  _DroidMediaBuffer(android::sp<android::GraphicBuffer>& buffer,
		   void *data,
		   DroidMediaCallback ref,
		   DroidMediaCallback unref);

  ~_DroidMediaBuffer();

  static void incRef(struct android_native_base_t* base);
  static void decRef(struct android_native_base_t* base);

  android::sp<android::GraphicBuffer> m_buffer;
  android::sp<DroidMediaBufferQueue> m_queue;

  uint32_t m_transform;
  uint32_t m_scalingMode;
  int64_t m_timestamp;
  uint64_t m_frameNumber;

  android::Rect m_crop;

  int m_slot;
  void *m_data;
  DroidMediaCallback m_ref;
  DroidMediaCallback m_unref;
};

#endif /* DROID_MEDIA_BUFFER_H */
