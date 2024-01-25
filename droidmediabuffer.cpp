/*
 * Copyright (C) 2014-2015 Jolla Ltd.
 * Copyright (C) 2021 Open Mobile Platform LLC.
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

#include <inttypes.h>
#include "droidmediabuffer.h"
#include "private.h"

#include <cutils/atomic.h>

#undef LOG_TAG
#define LOG_TAG "DroidMediaBuffer"

_DroidMediaBuffer::_DroidMediaBuffer(DroidMediaBufferItem& buffer,
                                     android::sp<DroidMediaBufferQueue> queue) :
    m_buffer(buffer.mGraphicBuffer),
    m_queue(queue),
    m_refCount(0),
    m_transform(buffer.mTransform),
    m_scalingMode(buffer.mScalingMode),
    m_timestamp(buffer.mTimestamp),
    m_frameNumber(buffer.mFrameNumber),
    m_crop(buffer.mCrop),
#if ANDROID_MAJOR >= 6
    m_slot(buffer.mSlot),
#else
    m_slot(buffer.mBuf),
#endif
    m_userData(0)
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

_DroidMediaBuffer::_DroidMediaBuffer(android::sp<android::GraphicBuffer>& buffer) :
    m_buffer(buffer),
    m_refCount(0),
    m_transform(-1),
    m_scalingMode(-1),
    m_timestamp(-1),
    m_frameNumber(-1),
    m_slot(-1),
    m_userData(0)
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
  DroidMediaBuffer * const buffer = reinterpret_cast<DroidMediaBuffer *>(base);

  android_atomic_inc(&buffer->m_refCount);
}

void _DroidMediaBuffer::decRef(struct android_native_base_t* base)
{
  DroidMediaBuffer * const buffer = reinterpret_cast<DroidMediaBuffer *>(base);

  if (android_atomic_dec(&buffer->m_refCount) == 1) {
      delete buffer;
  }
}

void _DroidMediaBuffer::update(const DroidMediaBufferItem& buffer)
{
  m_transform = buffer.mTransform;
  m_scalingMode = buffer.mScalingMode;
  m_timestamp = buffer.mTimestamp;
  m_frameNumber = buffer.mFrameNumber;
  m_crop = buffer.mCrop;
}

extern "C" {

DroidMediaBuffer *droid_media_buffer_create(uint32_t w, uint32_t h,
                                            uint32_t format)
{
  android::sp<android::GraphicBuffer>
    buffer(new android::GraphicBuffer(w, h, format,
				      android::GraphicBuffer::USAGE_HW_TEXTURE));

  android::status_t err = buffer->initCheck();

  if (err != android::NO_ERROR) {
    ALOGE("Error 0x%x allocating buffer", -err);
    buffer.clear();
    return NULL;
  }

  DroidMediaBuffer *droidBuffer = new DroidMediaBuffer(buffer);
  droidBuffer->incStrong(0);
  return droidBuffer;
}

void droid_media_buffer_destroy(DroidMediaBuffer *buffer)
{
#if ANDROID_MAJOR > 5
  if (buffer->m_queue != NULL) {
    buffer->m_queue->releaseBufferResources(buffer);
    return;
  }
  ALOGW("Destroying buffer refs but no queue %" PRIxPTR, (uintptr_t)buffer);
#else
  buffer->decStrong(0);
#endif
}

void droid_media_buffer_set_user_data(DroidMediaBuffer *buffer, void *data)
{
  buffer->m_userData = data;
}

void *droid_media_buffer_get_user_data(DroidMediaBuffer *buffer)
{
  return buffer->m_userData;
}

void droid_media_buffer_release(DroidMediaBuffer *buffer,
                                EGLDisplay display, EGLSyncKHR fence)
{
    if (buffer->m_queue == NULL) {
      return;
    }

    buffer->m_queue->releaseMediaBuffer(buffer, display, fence);
}

void *droid_media_buffer_lock(DroidMediaBuffer *buffer, uint32_t flags)
{
  int usage = 0;
  void *addr = NULL;
  android::status_t err;

  if (flags & DROID_MEDIA_BUFFER_LOCK_READ) {
    usage |= android::GraphicBuffer::USAGE_SW_READ_RARELY;
  }
  if (flags & DROID_MEDIA_BUFFER_LOCK_WRITE) {
    usage |= android::GraphicBuffer::USAGE_SW_WRITE_RARELY;
  }

  err = buffer->m_buffer->lock(usage, &addr);

  if (err != android::NO_ERROR) {
    ALOGE("Error 0x%x locking buffer", -err);
    return NULL;
  } else {
    return addr;
  }
}

bool droid_media_buffer_lock_ycbcr(DroidMediaBuffer *buffer,
                                   uint32_t flags,
                                   DroidMediaBufferYCbCr *ycbcr)
{
  int usage = 0;
  android_ycbcr droid_ycbcr;
  android::status_t err;

  if (!ycbcr) {
    ALOGE("No buffer for ycbcr data provided");
    return false;
  }

  if (flags & DROID_MEDIA_BUFFER_LOCK_READ) {
    usage |= android::GraphicBuffer::USAGE_SW_READ_RARELY;
  }
  if (flags & DROID_MEDIA_BUFFER_LOCK_WRITE) {
    usage |= android::GraphicBuffer::USAGE_SW_WRITE_RARELY;
  }

  err = buffer->m_buffer->lockYCbCr(usage, &droid_ycbcr);

  if (err != android::NO_ERROR) {
    ALOGE("Error 0x%x locking buffer", -err);
    return false;
  } else {
    ycbcr->y = droid_ycbcr.y;
    ycbcr->cb = droid_ycbcr.cb;
    ycbcr->cr = droid_ycbcr.cr;
    ycbcr->ystride = droid_ycbcr.ystride;
    ycbcr->cstride = droid_ycbcr.cstride;
    ycbcr->chroma_step = droid_ycbcr.chroma_step;
    return true;
  }
}

void droid_media_buffer_unlock(DroidMediaBuffer *buffer)
{
  android::status_t err = buffer->m_buffer->unlock();

  if (err != android::NO_ERROR) {
    ALOGE("Error 0x%x unlocking buffer", -err);
  }
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

const void *droid_media_buffer_get_handle(DroidMediaBuffer *buffer)
{
    return buffer->handle;
}

void droid_media_buffer_get_info(DroidMediaBuffer *buffer, DroidMediaBufferInfo *info)
{
    info->width = buffer->width;
    info->height = buffer->height;
    info->transform = buffer->m_transform;
    info->scaling_mode = buffer->m_scalingMode;
    info->timestamp = buffer->m_timestamp;
    info->frame_number = buffer->m_frameNumber;
    info->crop_rect = droid_media_buffer_get_crop_rect(buffer);
    info->format = buffer->format;
    info->stride = buffer->stride;
}

};
