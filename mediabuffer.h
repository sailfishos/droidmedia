#ifndef DROID_MEDIA_BUFFER_H
#define DROID_MEDIA_BUFFER_H

#include <system/window.h>
#include <gui/BufferQueue.h>

class DroidMediaBuffer : public ANativeWindowBuffer
{
public:
  DroidMediaBuffer(android::BufferQueue::BufferItem& buffer,
		   android::sp<android::BufferQueue>& queue,
		   void *data,
		   void (* ref)(void *m_data),
		   void (* unref)(void *m_data));

  ~DroidMediaBuffer();

  static void incRef(struct android_native_base_t* base);
  static void decRef(struct android_native_base_t* base);

  android::sp<android::GraphicBuffer> m_buffer;
  android::sp<android::BufferQueue> m_queue;

  uint32_t m_transform;
  uint32_t m_scalingMode;
  int64_t m_timestamp;
  uint64_t m_frameNumber;

  int m_slot;
  void *m_data;
  void (* m_ref)(void *m_data);
  void (* m_unref)(void *m_data);
};

#endif /* DROID_MEDIA_BUFFER_H */
