#ifndef DROID_MEDIA_BUFFER_H
#define DROID_MEDIA_BUFFER_H

#include <system/window.h>
#include <gui/BufferQueue.h>

class DroidMediaBuffer : public ANativeWindowBuffer
{
public:
  DroidMediaBuffer(android::BufferQueue::BufferItem& buffer,
		   void *data,
		   void (* ref)(void *m_data),
		   void (* unref)(void *m_data));

  ~DroidMediaBuffer();

  static void incRef(struct android_native_base_t* base);
  static void decRef(struct android_native_base_t* base);

  android::BufferQueue::BufferItem m_buffer;
  void *m_data;
  void (* m_ref)(void *m_data);
  void (* m_unref)(void *m_data);
};

#endif /* DROID_MEDIA_BUFFER_H */
