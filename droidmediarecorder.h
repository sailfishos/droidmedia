/*
 * Copyright (C) 2016 Jolla Ltd.
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

#ifndef DROID_MEDIA_RECORDER_H
#define DROID_MEDIA_RECORDER_H

#include "droidmedia.h"
#include "droidmediacamera.h"
#include "droidmediacodec.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DroidMediaRecorder DroidMediaRecorder;

DROID_MEDIA_EXPORT DroidMediaRecorder *droid_media_recorder_create(DroidMediaCamera *camera, DroidMediaCodecEncoderMetaData *meta);
DROID_MEDIA_EXPORT void droid_media_recorder_destroy(DroidMediaRecorder *recorder);

DROID_MEDIA_EXPORT bool droid_media_recorder_start(DroidMediaRecorder *recorder);
DROID_MEDIA_EXPORT void droid_media_recorder_stop(DroidMediaRecorder *recorder);

DROID_MEDIA_EXPORT void droid_media_recorder_set_data_callbacks(DroidMediaRecorder *recorder,
					     DroidMediaCodecDataCallbacks *cb, void *data);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_RECORDER_H */
