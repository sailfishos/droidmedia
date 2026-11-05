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

#ifndef DROID_MEDIA_SCREEN_CAPTURE_H
#define DROID_MEDIA_SCREEN_CAPTURE_H

#include "droidmedia.h"
#include "droidmediacodec.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Hardware H.264 encoder fed by an input Surface that start() registers with
 * sailfish.screencap; for a DISPLAY target, the compositor draws the display into it.
 * Single-use: new, [set_target], start, finish or stop, destroy.
 */
typedef struct _DroidMediaScreenCapture DroidMediaScreenCapture;

/* Who renders into the Surface */
typedef enum {
  /* The compositor mirrors the display */
  DROID_MEDIA_SCREEN_CAPTURE_TARGET_DISPLAY = 0,
  /* A renderer of its own; the compositor ignores the session */
  DROID_MEDIA_SCREEN_CAPTURE_TARGET_VIRTUAL = 1,
} DroidMediaScreenCaptureTarget;

/*
 * Callbacks are called on the encoder's output thread.
 * Must return promptly and must not call finish(), stop() or destroy().
 */
typedef struct {
  /* One access unit, or the SPS/PPS when codec_config is set. ts is the
   * presentation time the compositor gave the frame, in CLOCK_MONOTONIC ns */
  void (* data_available)(void *data, DroidMediaCodecData *encoded);
  void (* error)(void *data, int err);
  /* Called once, after the last access unit, if finish() reaches end of stream */
  void (* signal_eos)(void *data);
} DroidMediaScreenCaptureCallbacks;

/* Uses meta's parent.width, height and fps, bitrate and codec_specific.h264 (profile 0:
 * the encoder's). NULL if no hardware AVC encoder accepts them, or on Android < 8. */
DroidMediaScreenCapture *droid_media_screen_capture_new(DroidMediaCodecEncoderMetaData *meta,
                                                        DroidMediaScreenCaptureCallbacks *cb,
                                                        void *data);

/* Only before start(); the target defaults to DISPLAY. content_rect is where the picture
 * goes, all 0 for the whole frame. False for an unknown target or a rect outside the frame. */
bool droid_media_screen_capture_set_target(DroidMediaScreenCapture *capture,
                                           DroidMediaScreenCaptureTarget target,
                                           DroidMediaRect content_rect);

/* Starts the encoder, that publishes its input Surface to the compositor. */
bool droid_media_screen_capture_start(DroidMediaScreenCapture *capture);

/* Makes the next frame an IDR and asks the renderer for it, even on an idle screen.
 * Callable from any thread, also from the callbacks. Returns false unless started. */
bool droid_media_screen_capture_request_sync_frame(DroidMediaScreenCapture *capture);

/* Withdraws the Surface, delivers the remaining output and stops; timeout_ms <= 0 is 2000.
 * Returns true if the end of the stream was reached; the encoder is stopped either way. */
bool droid_media_screen_capture_finish(DroidMediaScreenCapture *capture, int timeout_ms);

/* Stops immediately and discards pending output. Idempotent; if finish() is
 * running on another thread, cuts it short and waits for it. */
void droid_media_screen_capture_stop(DroidMediaScreenCapture *capture);

/* Stops if needed, then frees the object. */
void droid_media_screen_capture_destroy(DroidMediaScreenCapture *capture);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_SCREEN_CAPTURE_H */
