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

#ifndef DROID_MEDIA_CONVERT_H
#define DROID_MEDIA_CONVERT_H

#include "droidmedia.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _DroidMediaConvert DroidMediaConvert;

DroidMediaConvert *droid_media_convert_create();
void droid_media_convert_destroy(DroidMediaConvert *convert);

bool droid_media_convert_to_i420(DroidMediaConvert *convert, DroidMediaData *in, void *out);
void droid_media_convert_set_crop_rect(DroidMediaConvert *convert, DroidMediaRect rect,
				       int32_t width, int32_t height);
bool droid_media_convert_is_i420(DroidMediaConvert *convert);

#ifdef __cplusplus
};
#endif

#endif /* DROID_MEDIA_CONVERT_H */
