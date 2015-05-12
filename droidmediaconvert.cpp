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

#include "droidmediaconvert.h"
#include "droidmediabuffer.h"
#include <media/editor/II420ColorConverter.h>
#include <cutils/log.h>
#include <dlfcn.h>

typedef void (*_getI420ColorConverter)(II420ColorConverter *converter);

extern "C" {

class DroidMediaConvert : public II420ColorConverter
{
public:
    DroidMediaConvert() :
        m_handle(NULL) {
      m_crop.top = m_crop.left = m_crop.bottom = m_crop.right = -1;
    }

    ~DroidMediaConvert() {
        if (m_handle) {
            dlclose(m_handle);
            m_handle = NULL;
        }
    }

    bool init() {
        if (m_handle) {
            ALOGW("DroidMediaConvert: already loaded");
            return true;
        }

        m_handle = dlopen("libI420colorconvert.so", RTLD_NOW);
        if (!m_handle) {
            ALOGE("DroidMediaConvert: failed to load libI420colorconvert.so. %s", dlerror());
            return false;
        }

        _getI420ColorConverter func = (_getI420ColorConverter)dlsym(m_handle, "getI420ColorConverter");
        if (!func) {
            ALOGE("DroidMediaConvert: failed to find symbol getI420ColorConverter");
            dlclose(m_handle);
            m_handle = NULL;
            return false;
        }

        func(this);

        return true;
    }

    void *m_handle;
    ARect m_crop;
};

DroidMediaConvert *droid_media_convert_create()
{
    DroidMediaConvert *conv = new DroidMediaConvert;
    if (conv->init()) {
        return conv;
    }

    delete conv;
    return NULL;
}

void droid_media_convert_destroy(DroidMediaConvert *convert)
{
    delete convert;
}

bool droid_media_convert_to_i420(DroidMediaConvert *convert, DroidMediaBuffer *in, void *out)
{
    void *addr = NULL;

    // TODO: I am not sure those flags are the optimum
    uint32_t usage = android::GraphicBuffer::USAGE_SOFTWARE_MASK | android::GraphicBuffer::USAGE_HW_MASK;

    if (convert->m_crop.left == -1 ||
	convert->m_crop.top == -1 ||
	convert->m_crop.right == -1 ||
	convert->m_crop.bottom == -1) {

      ALOGE("DroidMediaConvert: crop rectangle not set");

      return false;
    }

    android::status_t err = in->m_buffer->lock(usage, &addr);

    if (err != android::NO_ERROR) {
        ALOGE("DroidMediaConvert: error 0x%x locking buffer", -err);
        return false;
    }

    err = convert->convertDecoderOutputToI420(addr, in->m_buffer->getWidth(),
                                              in->m_buffer->getHeight(),
					      convert->m_crop,
                                              out);

    if (err != android::NO_ERROR) {
        ALOGE("DroidMediaConvert: error 0x%x converting buffer", -err);
        in->m_buffer->unlock();
        return false;
    }

    err = in->m_buffer->unlock();
    if (err != android::NO_ERROR) {
        ALOGE("DroidMediaConvert: error 0x%x unlocking buffer", -err);
        return false;
    }

    return true;
}

void droid_media_convert_set_crop_rect(DroidMediaConvert *convert, DroidMediaRect rect)
{
    convert->m_crop.left = rect.left;
    convert->m_crop.top = rect.top;
    convert->m_crop.right = rect.right - 1;
    convert->m_crop.bottom = rect.bottom - 1;
}
};
