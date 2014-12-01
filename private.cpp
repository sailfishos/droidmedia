#include "private.h"

BufferQueueListener::BufferQueueListener() :
    m_cb(0),
    m_data(0)
{

}

void BufferQueueListener::onFrameAvailable()
{
    if (m_cb && m_cb->frame_available) {
        m_cb->frame_available(m_data);
    }
}

void BufferQueueListener::onBuffersReleased()
{
    if (m_cb && m_cb->buffers_released) {
        m_cb->buffers_released(m_data);
    }
}

void BufferQueueListener::setCallbacks(DroidMediaRenderingCallbacks *cb, void *data) {
    m_cb = cb;
    m_data = data;
}
