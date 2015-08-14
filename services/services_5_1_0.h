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

    void binderDied(const wp<IBinder>& who) {
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

    sp<IBinder> createDisplay(const String8& displayName, bool secure) {
        return NULL;
    }

    void destroyDisplay(const sp<IBinder>& display) {
        // Nothing
    }

    virtual sp<IBinder> getBuiltInDisplay(int32_t id) {
        return NULL;
    }

    void setTransactionState(const Vector<ComposerState>& state,
			   const Vector<DisplayState>& displays, uint32_t flags) {
        // Nothing
    }

    void bootFinished() {
      // Nothing
    }

    bool authenticateSurfaceTexture(const sp<IGraphicBufferProducer>& surface) const {
        return true;
    }

    void setPowerMode(const sp<IBinder>& display, int mode) {

    }

    status_t getDisplayConfigs(const sp<IBinder>& display,
			       Vector<DisplayInfo>* configs) {
        return BAD_VALUE;
    }

    status_t getDisplayStats(const sp<IBinder>& display,
			     DisplayStatInfo* stats) {
        return BAD_VALUE;
    }

    int getActiveConfig(const sp<IBinder>& display) {
        return 0;
    }

    status_t setActiveConfig(const sp<IBinder>& display, int id) {
        return BAD_VALUE;
    }

    status_t captureScreen(const sp<IBinder>& display,
			   const sp<IGraphicBufferProducer>& producer,
			   Rect sourceCrop, uint32_t reqWidth, uint32_t reqHeight,
			   uint32_t minLayerZ, uint32_t maxLayerZ,
			   bool useIdentityTransform,
			   Rotation rotation) {
        return BAD_VALUE;
    }

    status_t clearAnimationFrameStats() {
        return BAD_VALUE;
    }

    status_t getAnimationFrameStats(FrameStats* outStats) const {
        return BAD_VALUE;
    }
};
