/*
 * Copyright (C) 2018 Jolla Ltd.
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

#ifdef USE_SERVICES_VENDOR_EXTENSION

#if ANDROID_MAJOR == 5 && ANDROID_MINOR == 1
#include "services_5_1_0_custom.h"
#else
#error "No droidmedia vendor extension defined for this Android version."
#endif

#else

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 1 && ANDROID_MICRO == 2
#include "services_4_1_2.h"
#endif

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 2 && ANDROID_MICRO == 2
#include "services_4_2_2.h"
#endif

#if ANDROID_MAJOR == 4 && ANDROID_MINOR == 4 && ANDROID_MICRO == 4
#include "services_4_4_4.h"
#endif

#if ANDROID_MAJOR == 5 && ANDROID_MINOR == 1
#include "services_5_1_0.h"
#endif

#if ANDROID_MAJOR == 6 && ANDROID_MINOR == 0
#include "services_6_0_0.h"
#endif

#if ANDROID_MAJOR == 7 && ANDROID_MINOR == 0
#include "services_7_0_0.h"
#endif

#if ANDROID_MAJOR == 7 && ANDROID_MINOR == 1
#include "services_7_1_0.h"
#endif

#if ANDROID_MAJOR == 8 && ANDROID_MINOR == 1
#include "services_8_1_0.h"
#endif

#if ANDROID_MAJOR == 9
#include "services_9_0_0.h"
#endif

#if ANDROID_MAJOR == 10
#include "services_10_0_0.h"
#endif

#if ANDROID_MAJOR == 11
#include "services_11_0_0.h"
#endif

#if ANDROID_MAJOR == 13
#include "services_13_0_0.h"
#endif

#endif
