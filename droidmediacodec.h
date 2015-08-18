/*
 * Copyright (C) 2014-2015 Jolla Ltd.
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
 *
 * Authored by: Mohammed Hassan <mohammed.hassan@jolla.com>
 */

#ifndef DROID_MEDIA_CODEC_H
#define DROID_MEDIA_CODEC_H

#include "droidmedia.h"
#include <sys/types.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DroidMediaCodec DroidMediaCodec;

typedef enum {
  DROID_MEDIA_CODEC_SW_ONLY = 0x1,
  DROID_MEDIA_CODEC_HW_ONLY = 0x2,
  DROID_MEDIA_CODEC_USE_EXTERNAL_LOOP = 0x4,
  DROID_MEDIA_CODEC_NO_MEDIA_BUFFER = 0x8,
} DroidMediaCodecFlags;

typedef enum {
  DROID_MEDIA_CODEC_LOOP_OK,
  DROID_MEDIA_CODEC_LOOP_ERROR,
  DROID_MEDIA_CODEC_LOOP_EOS,
} DroidMediaCodecLoopReturn;

typedef struct {
  const char *type;
  int32_t width;
  int32_t height;
  int32_t fps;
  int32_t channels;
  int32_t sample_rate;
  DroidMediaCodecFlags flags;
} DroidMediaCodecMetaData;

typedef struct {
  DroidMediaCodecMetaData parent;

  DroidMediaData codec_data;
} DroidMediaCodecDecoderMetaData;

typedef struct {
  DroidMediaCodecMetaData parent;

  int32_t color_format;
  int32_t bitrate;
  bool meta_data;
  int32_t stride;
  int32_t slice_height;
  int32_t max_input_size;
} DroidMediaCodecEncoderMetaData;

typedef struct {
  DroidMediaData data;
  int64_t ts;
  int64_t decoding_ts;
  bool sync; /* used for decoder input and encoder output */
  bool codec_config; /* user for encoder output */
} DroidMediaCodecData;

typedef struct {
  void (* signal_eos)(void *data);
  void (* error)(void *data, int err);
  int (* size_changed)(void *data, int32_t width, int32_t height);
} DroidMediaCodecCallbacks;

typedef struct {
  void (* data_available) (void *data, DroidMediaCodecData *encoded);
} DroidMediaCodecDataCallbacks;

DroidMediaBufferQueue *droid_media_codec_get_buffer_queue (DroidMediaCodec *codec);
DroidMediaCodec *droid_media_codec_create_decoder(DroidMediaCodecDecoderMetaData *meta);
DroidMediaCodec *droid_media_codec_create_encoder(DroidMediaCodecEncoderMetaData *meta);

void droid_media_codec_set_callbacks(DroidMediaCodec *codec, DroidMediaCodecCallbacks *cb, void *data);
void droid_media_codec_set_data_callbacks(DroidMediaCodec *codec,
					  DroidMediaCodecDataCallbacks *cb, void *data);

bool droid_media_codec_start(DroidMediaCodec *codec);
void droid_media_codec_stop(DroidMediaCodec *codec);
void droid_media_codec_destroy(DroidMediaCodec *codec);
void droid_media_codec_queue(DroidMediaCodec *codec, DroidMediaCodecData *data, DroidMediaBufferCallbacks *cb);
void droid_media_codec_flush(DroidMediaCodec *codec);
void droid_media_codec_drain(DroidMediaCodec *codec);
DroidMediaCodecLoopReturn droid_media_codec_loop(DroidMediaCodec *codec);
void droid_media_codec_get_output_info(DroidMediaCodec *codec, DroidMediaCodecMetaData *info, DroidMediaRect *crop);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_CODEC_H */
