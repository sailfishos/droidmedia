#include "mediabuffer.h"

DroidMediaBuffer::DroidMediaBuffer(android::BufferQueue::BufferItem& buffer,
                                   android::sp<android::BufferQueue>& queue,
                                   void *data,
                                   void (* ref)(void *m_data),
                                   void (* unref)(void *m_data)) :
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

DroidMediaBuffer::~DroidMediaBuffer()
{

}

void DroidMediaBuffer::incRef(struct android_native_base_t* base)
{
    DroidMediaBuffer *self = reinterpret_cast<DroidMediaBuffer *>(base);
    self->m_ref(self->m_data);
}

void DroidMediaBuffer::decRef(struct android_native_base_t* base)
{
    DroidMediaBuffer *self = reinterpret_cast<DroidMediaBuffer *>(base);
    self->m_unref(self->m_data);
}

extern "C" {
void droid_media_buffer_release(DroidMediaBuffer *buffer,
                                EGLDisplay display, EGLSyncKHR fence)
{

    int err = buffer->m_queue->releaseBuffer(buffer->m_slot, display, fence);
    if (err != android::NO_ERROR) {
        ALOGE("Error 0x%x releasing buffer", -err);
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

};
