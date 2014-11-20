#include <binder/IPCThreadState.h>
#include <binder/ProcessState.h>
#include <binder/IServiceManager.h>
#include <gui/ISurfaceComposer.h>
#include <gui/IDisplayEventConnection.h>
#include <CameraService.h>
#include <binder/MemoryHeapBase.h>
#include <MediaPlayerService.h>
#include "allocator.h"

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

    sp<IMemoryHeap> getCblk() const {
        return sp<IMemoryHeap>();
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

    void bootFinished() {
        // Nothing
    }

    bool authenticateSurfaceTexture(const sp<ISurfaceTexture>& surface) const {
        return false;
    }

    sp<IDisplayEventConnection> createDisplayEventConnection() {
        return sp<IDisplayEventConnection>();
    }
};

int
main(int argc, char* argv[])
{
    sp<ProcessState> proc(ProcessState::self());
    sp<IServiceManager> sm = defaultServiceManager();

    MiniSurfaceFlinger::instantiate();
    MediaPlayerService::instantiate();
    CameraService::instantiate();

    ProcessState::self()->startThreadPool();
    IPCThreadState::self()->joinThreadPool();

    return 0;
}
