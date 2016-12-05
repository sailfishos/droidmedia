/*
 * Copyright (C) 2014-2015-2015 Jolla Ltd.
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

#include <ui/Rect.h>

using namespace android;

class MiniSurfaceFlinger : public BinderService<MiniSurfaceFlinger>,
                           public BnSurfaceComposer,
                           public IBinder::DeathRecipient
{
public:
    static char const *getServiceName() {
        return "SurfaceFlinger";
    }

    void binderDied(const wp<IBinder>&) {
        // Nothing
    }

    sp<ISurfaceComposerClient> createConnection() {
        return sp<ISurfaceComposerClient>();
    }

    sp<IGraphicBufferAlloc> createGraphicBufferAlloc() {
        sp<DroidMediaAllocator> gba(new DroidMediaAllocator());
        return gba;
    }

    sp<IDisplayEventConnection> createDisplayEventConnection() {
        return sp<IDisplayEventConnection>();
    }

    sp<IBinder> createDisplay(const String8&, bool) {
        return NULL;
    }

    void destroyDisplay(const sp<IBinder>&) {
        // Nothing
    }

    virtual sp<IBinder> getBuiltInDisplay(int32_t) {
        return NULL;
    }

    void setTransactionState(const Vector<ComposerState>&, const Vector<DisplayState>&, uint32_t) {
        // Nothing
    }

    void bootFinished() {
      // Nothing
    }

    bool authenticateSurfaceTexture(const sp<IGraphicBufferProducer>&) const {
        return true;
    }

    void setPowerMode(const sp<IBinder>&, int) {

    }

    status_t getDisplayConfigs(const sp<IBinder>&, Vector<DisplayInfo>*) {
        return BAD_VALUE;
    }

    status_t getDisplayStats(const sp<IBinder>&, DisplayStatInfo*) {
        return BAD_VALUE;
    }

    int getActiveConfig(const sp<IBinder>&) {
        return 0;
    }

    status_t setActiveConfig(const sp<IBinder>&, int) {
        return BAD_VALUE;
    }

    status_t captureScreen(const sp<IBinder>&, const sp<IGraphicBufferProducer>&,
			   Rect, uint32_t, uint32_t, uint32_t, uint32_t,
			   bool, Rotation
#ifdef CM_BUILD
			   , bool
#endif
			   ) {
        return BAD_VALUE;
    }

    status_t clearAnimationFrameStats() {
        return BAD_VALUE;
    }

    status_t getAnimationFrameStats(FrameStats*) const {
        return BAD_VALUE;
    }
};
