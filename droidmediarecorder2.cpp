/*
 * Copyright (C) 2016 Jolla Ltd.
 * Copyright (C) 2026 Jolla Mobile Ltd
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
 * DroidMediaRecorder for the Camera2 backend. Recording is not implemented yet.
 */

#include <android/log.h>
#include "droidmediarecorder.h"
#include "private.h"

#undef LOG_TAG
#define LOG_TAG "DroidMediaRecorder2"

extern "C" {

DroidMediaRecorder *droid_media_recorder_create(DroidMediaCamera *camera, DroidMediaCodecEncoderMetaData *meta) {
  ALOGE("Recording is not supported yet");
  return NULL;
}

void droid_media_recorder_destroy(DroidMediaRecorder *recorder) {
}

bool droid_media_recorder_start(DroidMediaRecorder *recorder) {
  return false;
}

void droid_media_recorder_stop(DroidMediaRecorder *recorder) {
}

void droid_media_recorder_set_data_callbacks(DroidMediaRecorder *recorder,
					     DroidMediaCodecDataCallbacks *cb, void *data) {
}

};
