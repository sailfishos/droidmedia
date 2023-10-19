/*
 * Copyright (C) 2023 Jolla Ltd.
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

#ifndef DROID_MEDIA_PRIVATE2_H
#define DROID_MEDIA_PRIVATE2_H

#include "droidmediacamera.h"

#include <media/NdkMediaCodec.h>

bool droid_media_camera_start_external_recording(DroidMediaCamera *camera);

void droid_media_camera_stop_external_recording(DroidMediaCamera *camera);

ANativeWindow *droid_media_camera_get_external_video_window(DroidMediaCamera *camera);

bool droid_media_camera_set_external_video_window(DroidMediaCamera *camera, ANativeWindow *window);

bool droid_media_camera_remove_external_video_window(DroidMediaCamera *camera, ANativeWindow *window);

#endif /* DROID_MEDIA_PRIVATE2_H */
