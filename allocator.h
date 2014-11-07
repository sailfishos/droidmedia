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
