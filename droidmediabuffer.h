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

#if ANDROID_MAJOR >= 6
typedef android::BufferItem DroidMediaBufferItem;
#else
typedef android::BufferQueue::BufferItem DroidMediaBufferItem;
#endif

struct _DroidMediaBuffer : public ANativeWindowBuffer
{
public:
    _DroidMediaBuffer(DroidMediaBufferItem &buffer, android::sp<DroidMediaBufferQueue> queue);

    _DroidMediaBuffer(android::sp<android::GraphicBuffer> &buffer);

    ~_DroidMediaBuffer();

    static void incRef(struct android_native_base_t *base);
    static void decRef(struct android_native_base_t *base);

    void update(const DroidMediaBufferItem &buffer);

    android::sp<android::GraphicBuffer> m_buffer;
    android::sp<DroidMediaBufferQueue> m_queue;

    mutable volatile int32_t m_refCount;
    uint32_t m_transform;
    uint32_t m_scalingMode;
    int64_t m_timestamp;
    uint64_t m_frameNumber;

    android::Rect m_crop;

    int m_slot;
    void *m_userData;
};

#endif /* DROID_MEDIA_BUFFER_H */
