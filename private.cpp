#include "private.h"

BufferQueueListener::BufferQueueListener() :
    m_data(0)
{
    memset(&m_cb, 0x0, sizeof(m_cb));
}

void BufferQueueListener::onFrameAvailable()
{
    if (m_cb.frame_available) {
        m_cb.frame_available(m_data);
    }
}

void BufferQueueListener::onBuffersReleased()
{
    if (m_cb.buffers_released) {
        m_cb.buffers_released(m_data);
    }
}

void BufferQueueListener::setCallbacks(DroidMediaRenderingCallbacks *cb, void *data) {
    memcpy(&m_cb, cb, sizeof(m_cb));
    m_data = data;
}
