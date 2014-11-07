#include "allocator.h"
#include <ui/GraphicBuffer.h>

DroidMediaAllocator::DroidMediaAllocator() :
    m_size(0)
{

}

DroidMediaAllocator::~DroidMediaAllocator()
{

}

android::sp<android::GraphicBuffer>
DroidMediaAllocator::createGraphicBuffer(uint32_t w, uint32_t h,
                                         android::PixelFormat format, uint32_t usage,
                                         android::status_t* error)
{
    // Copied from SurfaceFlinger.cpp
    android::sp<android::GraphicBuffer>
        graphicBuffer(new android::GraphicBuffer(w, h, format,
                                                 usage, m_size));
    android::status_t err = graphicBuffer->initCheck();

    *error = err;

    if (err != android::NO_ERROR || graphicBuffer->handle == 0) {
        ALOGE("DroidMediaAllocator::createGraphicBuffer(w=%d, h=%d) "
              "failed (%s), handle=%p, format=0x%x, usage=0x%x, size=%d",
              w, h, strerror(-err), graphicBuffer->handle, format, usage, m_size);
        return 0;
    }

    return graphicBuffer;
}

void DroidMediaAllocator::setGraphicBufferSize(int size)
{
    m_size = size;
}
