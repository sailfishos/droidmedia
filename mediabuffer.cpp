#include "mediabuffer.h"

DroidMediaBuffer::DroidMediaBuffer(android::BufferQueue::BufferItem& buffer,
                                   android::sp<android::BufferQueue>& queue,
                                   void *data,
                                   void (* ref)(void *m_data),
                                   void (* unref)(void *m_data)) :
    m_buffer(buffer.mGraphicBuffer),
    m_queue(queue),
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

};
