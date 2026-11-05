/*
 * Copyright (C) 2026 Jolla Ltd.
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
 */

// A hand-written interface outside the AOSP allowlist of IInterface.h.
#define DO_NOT_CHECK_MANUAL_BINDER_INTERFACES

#include <inttypes.h>
#include <utils/Log.h>
#include <utils/Timers.h>
#include "screen_capture_service.h"

#undef LOG_TAG
#define LOG_TAG "ScreenCaptureService"

using namespace android;

namespace sailfish {

enum {
  GET_PRODUCER = IBinder::FIRST_CALL_TRANSACTION,
  REGISTER_PRODUCER,
  UNREGISTER_PRODUCER,
  REGISTER_LISTENER,
  REQUEST_FRAME,
};

enum {
  SESSION_CHANGED = IBinder::FIRST_CALL_TRANSACTION,
  FRAME_REQUESTED,
};

bool screenCaptureTargetValid(int width, int height, int target, const DroidMediaRect& rect)
{
  if (target != SCREEN_CAPTURE_TARGET_DISPLAY && target != SCREEN_CAPTURE_TARGET_VIRTUAL) {
    return false;
  }
  if (rect.left == 0 && rect.top == 0 && rect.right == 0 && rect.bottom == 0) {
    return true;
  }
  return rect.left >= 0 && rect.top >= 0 && rect.left < rect.right && rect.top < rect.bottom &&
    rect.right <= width && rect.bottom <= height;
}

static void writeSession(Parcel *parcel, const ScreenCaptureSession& session)
{
  parcel->writeInt64(session.generation);
  parcel->writeInt32(session.width);
  parcel->writeInt32(session.height);
  parcel->writeInt32(session.fps);
  parcel->writeInt32(session.target);
  parcel->writeInt32(session.content_rect.left);
  parcel->writeInt32(session.content_rect.top);
  parcel->writeInt32(session.content_rect.right);
  parcel->writeInt32(session.content_rect.bottom);
}

static ScreenCaptureSession readSession(const Parcel& parcel)
{
  ScreenCaptureSession session;
  session.generation = parcel.readInt64();
  session.width = parcel.readInt32();
  session.height = parcel.readInt32();
  session.fps = parcel.readInt32();
  session.target = parcel.readInt32();
  session.content_rect.left = parcel.readInt32();
  session.content_rect.top = parcel.readInt32();
  session.content_rect.right = parcel.readInt32();
  session.content_rect.bottom = parcel.readInt32();
  return session;
}

class BpScreenCaptureSessionListener : public BpInterface<IScreenCaptureSessionListener>
{
public:
  explicit BpScreenCaptureSessionListener(const sp<IBinder>& impl) :
    BpInterface<IScreenCaptureSessionListener>(impl) {
  }

  void sessionChanged(const ScreenCaptureSession& session) override {
    Parcel data, reply;
    data.writeInterfaceToken(IScreenCaptureSessionListener::getInterfaceDescriptor());
    writeSession(&data, session);
    // Oneway: the service never waits for the compositor.
    status_t err = remote()->transact(SESSION_CHANGED, data, &reply, IBinder::FLAG_ONEWAY);
    if (err != NO_ERROR) {
      ALOGW("sessionChanged failed: %d", err);
    }
  }

  void frameRequested(int64_t generation) override {
    Parcel data, reply;
    data.writeInterfaceToken(IScreenCaptureSessionListener::getInterfaceDescriptor());
    data.writeInt64(generation);
    status_t err = remote()->transact(FRAME_REQUESTED, data, &reply, IBinder::FLAG_ONEWAY);
    if (err != NO_ERROR) {
      ALOGW("frameRequested failed: %d", err);
    }
  }
};

IMPLEMENT_META_INTERFACE(ScreenCaptureSessionListener, "sailfish.IScreenCaptureSessionListener");

status_t BnScreenCaptureSessionListener::onTransact(uint32_t code, const Parcel& data,
                                             Parcel *reply, uint32_t flags)
{
  switch (code) {
  case SESSION_CHANGED:
    CHECK_INTERFACE(IScreenCaptureSessionListener, data, reply);
    sessionChanged(readSession(data));
    return NO_ERROR;
  case FRAME_REQUESTED:
    CHECK_INTERFACE(IScreenCaptureSessionListener, data, reply);
    frameRequested(data.readInt64());
    return NO_ERROR;
  default:
    return BBinder::onTransact(code, data, reply, flags);
  }
}

class BpScreenCaptureService : public BpInterface<IScreenCaptureService>
{
public:
  explicit BpScreenCaptureService(const sp<IBinder>& impl) :
    BpInterface<IScreenCaptureService>(impl) {
  }

  sp<IGraphicBufferProducer> getProducer(ScreenCaptureSession *session) override {
    Parcel data, reply;
    data.writeInterfaceToken(IScreenCaptureService::getInterfaceDescriptor());
    status_t err = remote()->transact(GET_PRODUCER, data, &reply);
    if (err != NO_ERROR) {
      ALOGE("getProducer failed: %d", err);
      *session = ScreenCaptureSession();
      return NULL;
    }

    sp<IGraphicBufferProducer> producer =
      interface_cast<IGraphicBufferProducer>(reply.readStrongBinder());
    *session = readSession(reply);
    return producer;
  }

  int64_t registerProducer(const sp<IGraphicBufferProducer>& producer,
                           const ScreenCaptureSession& session) override {
    Parcel data, reply;
    data.writeInterfaceToken(IScreenCaptureService::getInterfaceDescriptor());
    data.writeStrongBinder(IInterface::asBinder(producer));
    writeSession(&data, session);
    status_t err = remote()->transact(REGISTER_PRODUCER, data, &reply);
    if (err != NO_ERROR) {
      ALOGE("registerProducer failed: %d", err);
      return 0;
    }

    return reply.readInt64();
  }

  void unregisterProducer(int64_t generation) override {
    Parcel data, reply;
    data.writeInterfaceToken(IScreenCaptureService::getInterfaceDescriptor());
    data.writeInt64(generation);
    status_t err = remote()->transact(UNREGISTER_PRODUCER, data, &reply);
    if (err != NO_ERROR) {
      ALOGE("unregisterProducer failed: %d", err);
    }
  }

  status_t registerListener(const sp<IScreenCaptureSessionListener>& listener,
                            ScreenCaptureSession *session) override {
    Parcel data, reply;
    data.writeInterfaceToken(IScreenCaptureService::getInterfaceDescriptor());
    data.writeStrongBinder(IInterface::asBinder(listener));
    status_t err = remote()->transact(REGISTER_LISTENER, data, &reply);
    if (err == NO_ERROR) {
      err = reply.readInt32();
    }
    if (err == NO_ERROR) {
      *session = readSession(reply);
    }
    return err;
  }

  void requestFrame(int64_t generation) override {
    Parcel data, reply;
    data.writeInterfaceToken(IScreenCaptureService::getInterfaceDescriptor());
    data.writeInt64(generation);
    // Oneway: the recorder never waits for the service.
    status_t err = remote()->transact(REQUEST_FRAME, data, &reply, IBinder::FLAG_ONEWAY);
    if (err != NO_ERROR) {
      ALOGE("requestFrame failed: %d", err);
    }
  }
};

IMPLEMENT_META_INTERFACE(ScreenCaptureService, "sailfish.IScreenCaptureService");

status_t BnScreenCaptureService::onTransact(uint32_t code, const Parcel& data,
                                            Parcel *reply, uint32_t flags)
{
  switch (code) {
  case GET_PRODUCER: {
    CHECK_INTERFACE(IScreenCaptureService, data, reply);
    ScreenCaptureSession session;
    sp<IGraphicBufferProducer> producer = getProducer(&session);
    reply->writeStrongBinder(IInterface::asBinder(producer));
    writeSession(reply, session);
    return NO_ERROR;
  }
  case REGISTER_PRODUCER: {
    CHECK_INTERFACE(IScreenCaptureService, data, reply);
    sp<IGraphicBufferProducer> producer =
      interface_cast<IGraphicBufferProducer>(data.readStrongBinder());
    reply->writeInt64(registerProducer(producer, readSession(data)));
    return NO_ERROR;
  }
  case UNREGISTER_PRODUCER:
    CHECK_INTERFACE(IScreenCaptureService, data, reply);
    unregisterProducer(data.readInt64());
    return NO_ERROR;
  case REGISTER_LISTENER: {
    CHECK_INTERFACE(IScreenCaptureService, data, reply);
    sp<IScreenCaptureSessionListener> listener =
      interface_cast<IScreenCaptureSessionListener>(data.readStrongBinder());
    ScreenCaptureSession session = {};
    status_t err = registerListener(listener, &session);
    reply->writeInt32(err);
    writeSession(reply, session);
    return NO_ERROR;
  }
  case REQUEST_FRAME:
    CHECK_INTERFACE(IScreenCaptureService, data, reply);
    requestFrame(data.readInt64());
    return NO_ERROR;
  default:
    return BBinder::onTransact(code, data, reply, flags);
  }
}

// Generations start at the boot clock, so a restarted service never hands
// out one the compositor has already seen and possibly given up on.
ScreenCaptureService::ScreenCaptureService() :
  m_session(),
  m_generation(systemTime(SYSTEM_TIME_MONOTONIC)) {
}

sp<IGraphicBufferProducer> ScreenCaptureService::getProducer(ScreenCaptureSession *session)
{
  Mutex::Autolock lock(m_lock);
  *session = sessionLocked();
  return m_producer;
}

int64_t ScreenCaptureService::registerProducer(const sp<IGraphicBufferProducer>& producer,
                                               const ScreenCaptureSession& session)
{
  if (producer == NULL || session.width <= 0 || session.height <= 0 || session.fps <= 0 ||
      !screenCaptureTargetValid(session.width, session.height, session.target,
                                session.content_rect)) {
    ALOGE("Invalid session %dx%d @%d fps, target %d, content %d,%d-%d,%d",
          session.width, session.height, session.fps, session.target, session.content_rect.left,
          session.content_rect.top, session.content_rect.right, session.content_rect.bottom);
    return 0;
  }

  Mutex::Autolock lock(m_lock);
  clearLocked();

  // This service outlives recorders: end the session if the recorder dies.
  status_t err = IInterface::asBinder(producer)->linkToDeath(this);
  if (err != NO_ERROR) {
    ALOGE("Cannot watch the recorder: %d", err);
    notifyLocked();
    return 0;
  }

  m_producer = producer;
  m_session = session;
  m_session.generation = 0;
  m_generation++;
  ALOGI("Session %" PRId64 " started: %dx%d @%d fps, target %d, content %d,%d-%d,%d",
        m_generation, session.width, session.height, session.fps, session.target,
        session.content_rect.left, session.content_rect.top, session.content_rect.right,
        session.content_rect.bottom);
  notifyLocked();
  return m_generation;
}

void ScreenCaptureService::unregisterProducer(int64_t generation)
{
  Mutex::Autolock lock(m_lock);
  if (m_producer != NULL && generation == m_generation) {
    ALOGI("Session %" PRId64 " ended", generation);
    clearLocked();
    notifyLocked();
  }
}

status_t ScreenCaptureService::registerListener(const sp<IScreenCaptureSessionListener>& listener,
                                                ScreenCaptureSession *session)
{
  if (listener == NULL) {
    return BAD_VALUE;
  }

  Mutex::Autolock lock(m_lock);
  // Listeners are only removed when their process dies.
  status_t err = IInterface::asBinder(listener)->linkToDeath(this);
  if (err != NO_ERROR) {
    ALOGE("Cannot watch the listener: %d", err);
    return err;
  }

  m_listeners.push_back(listener);
  *session = sessionLocked();
  ALOGI("Listener added, %zu in total", m_listeners.size());
  return NO_ERROR;
}

void ScreenCaptureService::requestFrame(int64_t generation)
{
  Mutex::Autolock lock(m_lock);
  if (m_producer == NULL || generation != m_generation) {
    return;
  }

  // Under m_lock, like notifyLocked(), so it follows its generation's session change.
  for (const auto& listener : m_listeners) {
    listener->frameRequested(generation);
  }
}

void ScreenCaptureService::binderDied(const wp<IBinder>& who)
{
  Mutex::Autolock lock(m_lock);
  if (m_producer != NULL && IInterface::asBinder(m_producer).get() == who.unsafe_get()) {
    ALOGW("Recorder died, session %" PRId64 " ended", m_generation);
    clearLocked();
    notifyLocked();
    return;
  }

  for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it) {
    if (IInterface::asBinder(*it).get() == who.unsafe_get()) {
      m_listeners.erase(it);
      ALOGI("Listener died, %zu left", m_listeners.size());
      return;
    }
  }
}

ScreenCaptureSession ScreenCaptureService::sessionLocked() const
{
  ScreenCaptureSession session = m_session;
  session.generation = m_producer != NULL ? m_generation : 0;
  return session;
}

// Under m_lock, so oneway listener calls arrive in generation order.
void ScreenCaptureService::notifyLocked()
{
  const ScreenCaptureSession session = sessionLocked();
  for (const auto& listener : m_listeners) {
    listener->sessionChanged(session);
  }
}

void ScreenCaptureService::clearLocked()
{
  if (m_producer != NULL) {
    IInterface::asBinder(m_producer)->unlinkToDeath(this);
    m_producer.clear();
  }
  m_session = ScreenCaptureSession();
}

} // namespace sailfish
