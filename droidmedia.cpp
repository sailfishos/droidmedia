#include "droidmedia.h"
#include <binder/ProcessState.h>
#include <binder/IPCThreadState.h>

extern "C" {

void droid_media_init()
{
    android::ProcessState::self()->startThreadPool();
}

void droid_media_deinit()
{
    android::IPCThreadState::self()->stopProcess(false);
    android::IPCThreadState::self()->joinThreadPool();
}

};
