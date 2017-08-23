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

  sp<IBinder> createDisplay(const String8& displayName, bool secure) {
    return NULL;
  }

  void destroyDisplay(const sp<IBinder>& display) {
    // Nothing
  }

  void setTransactionState(const Vector<ComposerState>& state,
			   const Vector<DisplayState>& displays, uint32_t flags) {
    // Nothing
  }

  void blank(const sp<IBinder>& display) {
    // Nothing
  }

  void unblank(const sp<IBinder>& display) {
    // Nothing
  }

  virtual sp<IBinder> getBuiltInDisplay(int32_t id) {
    return NULL;
  }

  status_t getDisplayInfo(const sp<IBinder>& display, DisplayInfo* info) {
    return BAD_VALUE;
  }

bool authenticateSurfaceTexture(const sp<ISurfaceTexture>& surface) const {
    return true;
  }

#ifdef __arm__
 status_t captureScreen(const sp<IBinder>& display, sp<IMemoryHeap>* heap,
                           uint32_t* width, uint32_t* height, PixelFormat* format,
                           uint32_t reqWidth, uint32_t reqHeight,
                           uint32_t minLayerZ, uint32_t maxLayerZ) {
    return BAD_VALUE;
  }
#else
  bool isAnimationPermitted() {
    return false;
  }

 status_t captureScreen(const sp<IBinder>& display, sp<IMemoryHeap>* heap,
                           uint32_t* width, uint32_t* height, PixelFormat* format,
                           uint32_t reqWidth, uint32_t reqHeight,
                           uint32_t minLayerZ, uint32_t maxLayerZ) {
    return BAD_VALUE;
  }
#endif
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
