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
 * Records the screen to a raw H.264 file with droid_media_screen_capture_*,
 * without GStreamer. Needs qt5-qpa-hwcomposer-plugin with screen recording enabled.
 *
 * usage: screencap_surface_capture_test [width] [height] [bitrate] [fps]
 *                                       [seconds] [output]
 */

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "droidmediascreencapture.h"

struct RecordingState {
  int fd;
  int frames;
  size_t bytes;
  int64_t last_ts;
  bool error;
  bool eos;
};

static void on_data(void *data, DroidMediaCodecData *encoded)
{
  RecordingState *state = static_cast<RecordingState*>(data);

  if (write(state->fd, encoded->data.data, encoded->data.size) != encoded->data.size) {
    fprintf(stderr, "write failed: %s\n", strerror(errno));
    state->error = true;
    return;
  }
  state->bytes += encoded->data.size;

  if (!encoded->codec_config) {
    if (state->frames > 0 && encoded->ts < state->last_ts) {
      fprintf(stderr, "timestamp %" PRId64 " ns after %" PRId64 " ns\n",
              encoded->ts, state->last_ts);
      state->error = true;
    }
    state->last_ts = encoded->ts;
    state->frames++;
  }
}

static void on_error(void *data, int err)
{
  fprintf(stderr, "encoder error %d\n", err);
  static_cast<RecordingState*>(data)->error = true;
}

static void on_eos(void *data)
{
  static_cast<RecordingState*>(data)->eos = true;
}

int main(int argc, char **argv)
{
  int width = argc > 1 ? atoi(argv[1]) : 1080;
  int height = argc > 2 ? atoi(argv[2]) : 2520;
  int bitrate = argc > 3 ? atoi(argv[3]) : 8000000;
  int fps = argc > 4 ? atoi(argv[4]) : 30;
  int seconds = argc > 5 ? atoi(argv[5]) : 10;
  const char *path = argc > 6 ? argv[6] : "/tmp/screencap.h264";

  if (width <= 0 || height <= 0 || bitrate <= 0 || fps <= 0 || seconds <= 0) {
    fprintf(stderr, "usage: %s [width] [height] [bitrate] [fps] [seconds] [output]\n",
            argv[0]);
    return 2;
  }

  RecordingState state = {};
  state.fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (state.fd < 0) {
    fprintf(stderr, "cannot open %s: %s\n", path, strerror(errno));
    return 1;
  }

  DroidMediaScreenCaptureCallbacks cb = {};
  cb.data_available = on_data;
  cb.error = on_error;
  cb.signal_eos = on_eos;

  DroidMediaCodecEncoderMetaData meta = {};
  meta.parent.width = width;
  meta.parent.height = height;
  meta.parent.fps = fps;
  meta.bitrate = bitrate;

  DroidMediaScreenCapture *capture = droid_media_screen_capture_new(&meta, &cb, &state);
  if (!capture || !droid_media_screen_capture_start(capture)) {
    fprintf(stderr, "cannot start screen capture\n");
    if (capture) {
      droid_media_screen_capture_destroy(capture);
    }
    close(state.fd);
    return 1;
  }

  fprintf(stderr, "recording %dx%d @%d fps for %d s to %s\n", width, height, fps, seconds, path);
  sleep(seconds);

  bool finished = droid_media_screen_capture_finish(capture, 3000);
  fprintf(stderr, "graceful finish: %s\n", finished ? "reached codec EOS" : "timed out");
  droid_media_screen_capture_destroy(capture);
  close(state.fd);

  fprintf(stderr, "%d frames, %zu bytes, eos callback %d, error %d\n", state.frames,
          state.bytes, state.eos, state.error);
  return state.error || state.frames == 0 || !finished ? 1 : 0;
}
