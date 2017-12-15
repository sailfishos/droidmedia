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

#include "allocator.h"
#include <ui/GraphicBuffer.h>

#define LOG_TAG "DroidMediaAllocator"

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
#if ANDROID_MAJOR >= 7
                                                          std::string requestorName,
#endif
                                         android::status_t* error)
{
    // Copied from SurfaceFlinger.cpp
    android::sp<android::GraphicBuffer>
#if (ANDROID_MAJOR == 4 && ANDROID_MINOR < 2)
    graphicBuffer(new android::GraphicBuffer(w, h, format,
                                             usage, m_size));
#else
    graphicBuffer(new android::GraphicBuffer(w, h, format,
                                             usage));
#endif
    android::status_t err = graphicBuffer->initCheck();

    *error = err;

    if (err != android::NO_ERROR || graphicBuffer->handle == 0) {
        ALOGE("createGraphicBuffer(w=%d, h=%d) "
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
