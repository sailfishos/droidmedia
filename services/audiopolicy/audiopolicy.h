/*
 * Copyright (C) 2022 Jolla Ltd.
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
 */

#if ANDROID_MAJOR == 5 && ANDROID_MINOR == 1
#include "audiopolicy_5_1_0.h"
#endif

#if ANDROID_MAJOR == 6 && ANDROID_MINOR == 0
#include "audiopolicy_6_0_0.h"
#endif

#if ANDROID_MAJOR == 7
#include "audiopolicy_7_x_x.h"
#endif

#if ANDROID_MAJOR == 8 && ANDROID_MINOR == 1
#include "audiopolicy_8_1_0.h"
#endif

#if ANDROID_MAJOR == 9
#include "audiopolicy_9_0_0.h"
#endif

#if ANDROID_MAJOR == 10
#include "audiopolicy_10_0_0.h"
#endif

#if ANDROID_MAJOR == 11
#include "audiopolicy_11_0_0.h"
#endif

#if ANDROID_MAJOR == 13
#include "audiopolicy_13_0_0.h"
#endif
