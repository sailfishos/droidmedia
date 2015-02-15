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

#ifndef DROID_MEDIA_ALLOCATOR_H
#define DROID_MEDIA_ALLOCATOR_H

#include <gui/IGraphicBufferAlloc.h>

class DroidMediaAllocator : public android::BnGraphicBufferAlloc
{
public:
  DroidMediaAllocator();
  ~DroidMediaAllocator();

  android::sp<android::GraphicBuffer> createGraphicBuffer(uint32_t w, uint32_t h,
							  android::PixelFormat format, uint32_t usage,
							  android::status_t* error);

  void setGraphicBufferSize(int size);

private:
  int m_size;
};

#endif /* DROID_MEDIA_ALLOCATOR_H */
