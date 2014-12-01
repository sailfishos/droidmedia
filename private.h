#ifndef DROID_MEDIA_PRIVATE_H
#define DROID_MEDIA_PRIVATE_H

#include "droidmedia.h"
#include <gui/BufferQueue.h>

class BufferQueueListener : public android::BufferQueue::ConsumerListener {
public:
  BufferQueueListener();
  void onFrameAvailable();
  void onBuffersReleased();
  void setCallbacks(DroidMediaRenderingCallbacks *cb, void *data);

private:
    DroidMediaRenderingCallbacks *m_cb;
    void *m_data;
};

#endif /* DROID_MEDIA_PRIVATE_H */
