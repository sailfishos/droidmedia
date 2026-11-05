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

/*
 * Screen capture entry points for qt5-qpa-hwcomposer-plugin, resolved with android_dlsym().
 * They run on the compositor's render thread; the callback runs on a Binder thread.
 */

#include <inttypes.h>
#include <binder/IServiceManager.h>
#include <binder/ProcessState.h>
#include <gui/Surface.h>
#include <utils/Log.h>
#include <utils/Mutex.h>
#include <utils/Timers.h>
#include "screen_capture_service.h"

#undef LOG_TAG
#define LOG_TAG "MinisfScreenCapture"

using namespace android;
using namespace sailfish;

/* Callback events */
enum {
  MINISF_SCREEN_CAPTURE_SESSION_CHANGED = 1,
  MINISF_SCREEN_CAPTURE_FRAME_REQUESTED = 2,
};

typedef void (*MinisfScreenCaptureCallback)(void *user, int event);

struct MinisfScreenCaptureTarget {
  sp<Surface> surface;
  uint64_t generation;
};

// While the service is missing, look it up at most this often.
#define CONNECT_RETRY_MS 250

// s_lock guards the state below.
static Mutex s_lock{};
static sp<IScreenCaptureService> s_service{};
static ScreenCaptureSession s_session{};
static nsecs_t s_nextConnectNs{};

// s_callbackLock guards the callback and is held while it runs, so that
// clearing the callback waits for a running one. Never taken under s_lock.
static Mutex s_callbackLock{};
static MinisfScreenCaptureCallback s_callback{};
static void *s_callbackUser{};

static void notify(int event)
{
  Mutex::Autolock lock(s_callbackLock);
  if (s_callback) {
    s_callback(s_callbackUser, event);
  }
}

/* Receives session changes, frame requests and the death of minisfservice on
 * Binder threads. */
class SessionListener : public BnScreenCaptureSessionListener,
                        public IBinder::DeathRecipient
{
public:
  void sessionChanged(const ScreenCaptureSession& session) override {
    {
      Mutex::Autolock lock(s_lock);
      s_session = session;
    }
    notify(MINISF_SCREEN_CAPTURE_SESSION_CHANGED);
  }

  void frameRequested(int64_t generation) override {
    {
      Mutex::Autolock lock(s_lock);
      if (generation == 0 || s_session.generation != generation) {
        return;
      }
    }
    notify(MINISF_SCREEN_CAPTURE_FRAME_REQUESTED);
  }

  void binderDied(const wp<IBinder>& who) override {
    {
      Mutex::Autolock lock(s_lock);
      if (s_service == NULL || IInterface::asBinder(s_service).get() != who.unsafe_get()) {
        return;
      }
      ALOGW("%s died", ScreenCaptureService::getServiceName());
      s_service.clear();
      s_session = {};
    }
    notify(MINISF_SCREEN_CAPTURE_SESSION_CHANGED);
  }
};

static sp<SessionListener> s_listener;

/* Looks up the service and subscribes to its session changes. */
static bool connectLocked()
{
  if (s_service != NULL) {
    return true;
  }

  const nsecs_t now = systemTime(SYSTEM_TIME_MONOTONIC);
  if (now < s_nextConnectNs) {
    return false;
  }
  s_nextConnectNs = now + ms2ns(CONNECT_RETRY_MS);

  // Session changes, and the encoder's queue calling the producer listener
  // that Surface serves from this process, need Binder threads.
  ProcessState::self()->startThreadPool();

  // Don't getService(): it waits up to 5 s for a missing service.
  sp<IBinder> binder =
    defaultServiceManager()->checkService(String16(ScreenCaptureService::getServiceName()));
  if (binder == NULL) {
    return false;
  }

  if (s_listener == NULL) {
    s_listener = new SessionListener;
  }
  if (binder->linkToDeath(s_listener) != NO_ERROR) {
    return false;
  }

  // The service only makes oneway calls to the listener, so a change that
  // races this call waits for s_lock and lands after the snapshot.
  sp<IScreenCaptureService> service = interface_cast<IScreenCaptureService>(binder);
  ScreenCaptureSession session;
  status_t err = service->registerListener(s_listener, &session);
  if (err != NO_ERROR) {
    ALOGE("Cannot subscribe to %s: %d", ScreenCaptureService::getServiceName(), err);
    binder->unlinkToDeath(s_listener);
    return false;
  }

  ALOGI("Subscribed to %s", ScreenCaptureService::getServiceName());
  s_service = service;
  s_session = session;
  return true;
}

extern "C" {

/* Returns 0 and a zeroed session when there is none. */
int minisf_screen_capture_session_query(ScreenCaptureSession *info)
{
  Mutex::Autolock lock(s_lock);
  if (!connectLocked() || s_session.generation == 0) {
    *info = ScreenCaptureSession();
    return 0;
  }

  *info = s_session;
  return 1;
}

/* Sets the callback for MINISF_SCREEN_CAPTURE_* events, called on a Binder thread. It may call
 * the query, but not this function; passing NULL waits for a running callback to return. */
void minisf_screen_capture_set_callback(MinisfScreenCaptureCallback callback, void *user)
{
  Mutex::Autolock lock(s_callbackLock);
  s_callback = callback;
  s_callbackUser = user;
}

void *minisf_screen_capture_target_acquire_generation(uint64_t generation)
{
  sp<IScreenCaptureService> service;
  {
    Mutex::Autolock lock(s_lock);
    service = s_service;
  }
  if (service == NULL) {
    return NULL;
  }

  ScreenCaptureSession current;
  sp<IGraphicBufferProducer> producer = service->getProducer(&current);
  if (producer == NULL || (uint64_t)current.generation != generation) {
    return NULL;
  }

  MinisfScreenCaptureTarget *target = new MinisfScreenCaptureTarget;
  target->surface = new Surface(producer, true);
  target->generation = generation;
  ALOGI("Attached to session %" PRIu64, generation);
  return target;
}

void *minisf_screen_capture_target_native_window(void *target)
{
  if (!target) {
    return NULL;
  }

  // RefBase, not ANativeWindow, is Surface's primary base: convert to the
  // ANativeWindow subobject before the pointer loses its type.
  ANativeWindow *window = static_cast<MinisfScreenCaptureTarget *>(target)->surface.get();
  return window;
}

void minisf_screen_capture_target_release(void *target)
{
  MinisfScreenCaptureTarget *t = static_cast<MinisfScreenCaptureTarget *>(target);
  if (t) {
    ALOGI("Detached from session %" PRIu64, t->generation);
    delete t;
  }
}

}
