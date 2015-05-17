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

#include "droidmediabuffer.h"
#include "private.h"

_DroidMediaBuffer::_DroidMediaBuffer(android::BufferQueue::BufferItem& buffer,
				     android::sp<DroidMediaBufferQueue> queue,
				     void *data,
				     DroidMediaCallback ref,
				     DroidMediaCallback unref) :
    m_buffer(buffer.mGraphicBuffer),
    m_queue(queue),
    m_transform(buffer.mTransform),
    m_scalingMode(buffer.mScalingMode),
    m_timestamp(buffer.mTimestamp),
    m_frameNumber(buffer.mFrameNumber),
    m_crop(buffer.mCrop),
    m_slot(buffer.mBuf),
    m_data(data),
    m_ref(ref),
    m_unref(unref)
{
    width  = buffer.mGraphicBuffer->width;
    height = buffer.mGraphicBuffer->height;
    stride = buffer.mGraphicBuffer->stride;
    format = buffer.mGraphicBuffer->format;
    usage  = buffer.mGraphicBuffer->usage;
    handle = buffer.mGraphicBuffer->handle;

    common.incRef = incRef;
    common.decRef = decRef;
}

_DroidMediaBuffer::_DroidMediaBuffer(android::sp<android::GraphicBuffer>& buffer,
				     void *data,
				     DroidMediaCallback ref,
				     DroidMediaCallback unref) :
    m_buffer(buffer),
    m_transform(-1),
    m_scalingMode(-1),
    m_timestamp(-1),
    m_frameNumber(-1),
    m_slot(-1),
    m_data(data),
    m_ref(ref),
    m_unref(unref)
{
    width  = m_buffer->width;
    height = m_buffer->height;
    stride = m_buffer->stride;
    format = m_buffer->format;
    usage  = m_buffer->usage;
    handle = m_buffer->handle;

    common.incRef = incRef;
    common.decRef = decRef;
}

_DroidMediaBuffer::~_DroidMediaBuffer()
{

}

void _DroidMediaBuffer::incRef(struct android_native_base_t* base)
{
    DroidMediaBuffer *self = reinterpret_cast<DroidMediaBuffer *>(base);
    self->m_ref(self->m_data);
}

void _DroidMediaBuffer::decRef(struct android_native_base_t* base)
{
    DroidMediaBuffer *self = reinterpret_cast<DroidMediaBuffer *>(base);
    self->m_unref(self->m_data);
}

extern "C" {
DroidMediaBuffer *droid_media_buffer_create_from_yv12_data(uint32_t w, uint32_t h,
							   DroidMediaData *data,
							   DroidMediaBufferCallbacks *cb)
{
  android::sp<android::GraphicBuffer>
    buffer(new android::GraphicBuffer(w, h, HAL_PIXEL_FORMAT_YV12,
				      android::GraphicBuffer::USAGE_HW_TEXTURE));

  android::status_t err = buffer->initCheck();

  if (err != android::NO_ERROR) {
    ALOGE("DroidMediaBuffer: Error 0x%x allocating buffer", -err);
    buffer.clear();
    return NULL;
  }

  void *addr = NULL;

  err = buffer->lock(android::GraphicBuffer::USAGE_SW_READ_RARELY
		     | android::GraphicBuffer::USAGE_SW_WRITE_RARELY, &addr);
  if (err != android::NO_ERROR) {
    ALOGE("DroidMediaBuffer: Error 0x%x locking buffer", -err);
    buffer.clear();
    return NULL;
  }

  memcpy(addr, data->data, data->size);

  err = buffer->unlock();
  if (err != android::NO_ERROR) {
    ALOGE("DroidMediaBuffer: Error 0x%x unlocking buffer", -err);
    buffer.clear();
    return NULL;
  }

  return new DroidMediaBuffer(buffer, cb->data, cb->ref, cb->unref);
}

void droid_media_buffer_release(DroidMediaBuffer *buffer,
                                EGLDisplay display, EGLSyncKHR fence)
{
    if (buffer->m_queue == NULL) {
      // TODO: what should we do with fence?
      delete buffer;
      return;
    }

    int err = buffer->m_queue->releaseBuffer(buffer->m_slot,
#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4 // TODO: fix this when we do video rendering
					     buffer->m_frameNumber,
#endif
					     display, fence
#if ANDROID_MAJOR == 4 && (ANDROID_MINOR == 4 || ANDROID_MINOR == 2) // TODO: fix this when we do video rendering
					     , android::Fence::NO_FENCE
#endif
);
    switch (err) {
    case android::NO_ERROR:
        break;

    case android::BufferQueue::STALE_BUFFER_SLOT:
        ALOGW("DroidMediaBuffer: Released stale buffer %d", buffer->m_slot);
        break;

    default:
        ALOGE("DroidMediaBuffer: Error 0x%x releasing buffer %d", -err, buffer->m_slot);
        break;
    }

    delete buffer;
}

uint32_t droid_media_buffer_get_transform(DroidMediaBuffer * buffer)
{
    return buffer->m_transform;
}

uint32_t droid_media_buffer_get_scaling_mode(DroidMediaBuffer * buffer)
{
    return buffer->m_scalingMode;
}

int64_t droid_media_buffer_get_timestamp(DroidMediaBuffer * buffer)
{
    return buffer->m_timestamp;
}

uint64_t droid_media_buffer_get_frame_number(DroidMediaBuffer * buffer)
{
    return buffer->m_frameNumber;
}

DroidMediaRect droid_media_buffer_get_crop_rect(DroidMediaBuffer * buffer)
{
    DroidMediaRect rect;
    rect.left = buffer->m_crop.left;
    rect.right = buffer->m_crop.right;
    rect.top = buffer->m_crop.top;
    rect.bottom = buffer->m_crop.bottom;

    return rect;
}

uint32_t droid_media_buffer_get_width(DroidMediaBuffer * buffer)
{
    return buffer->width;
}

uint32_t droid_media_buffer_get_height(DroidMediaBuffer * buffer)
{
    return buffer->height;
}

};
