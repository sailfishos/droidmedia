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

    void bootFinished() {
        // Nothing
    }

    sp<IDisplayEventConnection> createDisplayEventConnection() {
        return sp<IDisplayEventConnection>();
    }

    sp<IMemoryHeap> getCblk() const {
        static android::sp<android::MemoryHeapBase>
            mem(new MemoryHeapBase(4096,
                                   MemoryHeapBase::READ_ONLY, "SurfaceFlinger read-only heap"));
        return mem;
    }

    void setTransactionState(const Vector<ComposerState>& state,
                             int orientation, uint32_t flags) {
        // Nothing
    }

    status_t captureScreen(DisplayID dpy, sp<IMemoryHeap> *heap,
                           uint32_t* width, uint32_t* height, PixelFormat* format,
                           uint32_t reqWidth, uint32_t reqHeight,
                           uint32_t minLayerZ, uint32_t maxLayerZ) {
        return BAD_VALUE;
    }

    virtual status_t turnElectronBeamOff(int32_t mode) {
        return BAD_VALUE;
    }

    virtual status_t turnElectronBeamOn(int32_t mode) {
        return BAD_VALUE;
    }

    bool authenticateSurfaceTexture(const sp<ISurfaceTexture>& surface) const {
        return false;
    }
};

class FakePermissionController : public BinderService<FakePermissionController>,
                                 public BnPermissionController
{
public:
    static char const *getServiceName() {
        return "permission";
    }

    bool checkPermission(const String16& permission, int32_t, int32_t) {
        if (permission == String16("android.permission.CAMERA")) {
            return true;
        }

        return false;
    }

};
