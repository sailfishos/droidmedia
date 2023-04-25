LOCAL_PATH:= $(call my-dir)

ANDROID_MAJOR :=
ANDROID_MINOR :=
ANDROID_MICRO :=
FORCE_HAL_PARAM :=

include $(LOCAL_PATH)/env.mk
ifdef FORCE_HAL
FORCE_HAL_PARAM := -DFORCE_HAL=$(FORCE_HAL)
endif
ifdef LEGACY_ANDROID_13_REVISION
LEGACY_ANDROID_13_PARAM := -DLEGACY_ANDROID_13_REVISION=$(LEGACY_ANDROID_13_REVISION)
endif

ifndef ANDROID_MAJOR
# First check if version_defaults.mk was already loaded.
ifndef PLATFORM_VERSION
include build/core/version_defaults.mk
endif
ifeq ($(strip $(PLATFORM_VERSION)),)
$error(*** Cannot get Android platform version)
endif
ANDROID_MAJOR = $(shell echo $(PLATFORM_VERSION) | cut -d . -f 1)
ANDROID_MINOR = $(shell echo $(PLATFORM_VERSION) | cut -d . -f 2)
ANDROID_MICRO = $(shell echo $(PLATFORM_VERSION) | cut -d . -f 3)
endif

ifeq ($(strip $(ANDROID_MAJOR)),)
$(error *** ANDROID_MAJOR undefined)
endif

ifeq ($(strip $(ANDROID_MINOR)),)
$(error *** ANDROID_MINOR undefined)
endif

ifeq ($(strip $(ANDROID_MINOR)),)
$(warning *** ANDROID_MICRO undefined. Assuming 0)
ANDROID_MINOR = 0
endif

ifeq ($(strip $(ANDROID_MICRO)),)
$(warning *** ANDROID_MICRO undefined. Assuming 0)
ANDROID_MICRO = 0
endif

ifeq ($(shell test $(ANDROID_MAJOR) -le 11 && echo true),true)
ifneq (,$(wildcard frameworks/av/media/mediaserver/Android.mk))
DROIDMEDIA_32 := $(shell cat frameworks/av/media/mediaserver/Android.mk |grep "LOCAL_32_BIT_ONLY[[:space:]]*:=[[:space:]]*" |grep -o "true\|1\|false\|0")
else
ifeq ($(shell test $(ANDROID_MAJOR) -le 11 && echo true),true)
ifneq (,$(wildcard frameworks/av/media/mediaserver/Android.bp))
DROIDMEDIA_32 := $(shell cat frameworks/av/media/mediaserver/Android.bp | grep compile_multilib | grep -wo "32" | sed "s/32/true/")
endif
else
ifneq (,$(wildcard frameworks/av/media/mediaserver/Android.bp))
DROIDMEDIA_32 := $(shell cat frameworks/av/media/mediaserver/Android.bp | grep "^    compile_multilib" | grep -wo "prefer32" | sed "s/prefer32/true/")
endif
endif
endif
endif


include $(CLEAR_VARS)
LOCAL_SRC_FILES := droidmedia.cpp \
                   droidmediacamera.cpp \
                   droidmediaconstants.cpp \
                   droidmediacodec.cpp \
                   droidmediaconvert.cpp \
                   droidmediarecorder.cpp \
                   allocator.cpp \
                   droidmediabuffer.cpp \
                   private.cpp

ifeq ($(shell test $(ANDROID_MAJOR) -ge 7 && echo true),true)
LOCAL_SRC_FILES += AsyncCodecSource.cpp
endif

LOCAL_SHARED_LIBRARIES := libc \
                          libdl \
                          libutils \
                          libcutils \
                          libcamera_client \
                          libgui \
                          libui \
                          libbinder \
                          libstagefright \
                          libstagefright_foundation \
                          libmedia

ifeq ($(shell test $(ANDROID_MAJOR) -ge 8 && echo true),true)
LOCAL_SHARED_LIBRARIES += liblog
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),9))
LOCAL_SHARED_LIBRARIES += libmediaextractor
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 9 && echo true),true)
LOCAL_SHARED_LIBRARIES += android.hidl.memory@1.0 \
                          libmedia_omx
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 11 && echo true),true)
LOCAL_SHARED_LIBRARIES += libmedia_codeclist
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 12 && echo true),true)
LOCAL_SHARED_LIBRARIES += libactivitymanager_aidl \
                          libbatterystats_aidl \
                          libmediautils \
                          libpermission
endif

LOCAL_CPPFLAGS=-DANDROID_MAJOR=$(ANDROID_MAJOR) -DANDROID_MINOR=$(ANDROID_MINOR) -DANDROID_MICRO=$(ANDROID_MICRO) $(FORCE_HAL_PARAM) $(LEGACY_ANDROID_13_PARAM) -Wno-unused-parameter
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libdroidmedia

ifeq ($(shell test $(ANDROID_MAJOR) -ge 7 && echo true),true)
LOCAL_C_INCLUDES := frameworks/native/include/media/openmax \
                    frameworks/native/include/media/hardware
ifeq ($(shell test $(ANDROID_MAJOR) -ge 8 && echo true),true)
LOCAL_C_INCLUDES += frameworks/native/libs/nativewindow/include \
                    frameworks/av/media/libstagefright/omx/include \
                    frameworks/av/media/libstagefright/xmlparser/include
endif
ifeq ($(shell test $(ANDROID_MAJOR) -ge 11 && echo true),true)
LOCAL_C_INCLUDES += frameworks/av/media/libmediametrics/include \
                    frameworks/av/media/libstagefright/include \
                    frameworks/av/drm/libmediadrm/interface
endif
else
LOCAL_C_INCLUDES := frameworks/native/include/media/openmax
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),9))
LOCAL_C_INCLUDES += frameworks/av/media/libmediaextractor/include
endif

include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := minimedia.cpp
LOCAL_C_INCLUDES := frameworks/av/services/camera/libcameraservice \
                    system/media/camera/include \
		    $(call include-path-for, audio-utils) \
		    frameworks/av/media/libaaudio/include \
		    frameworks/av/media/libaaudio/src \
		    frameworks/av/media/libaaudio/src/binding \
		    frameworks/av/media/libmedia \
		    frameworks/av/services/audioflinger

LOCAL_SHARED_LIBRARIES := libcameraservice \
                          libcamera_client \
                          libutils \
                          libmedia \
                          libbinder \
                          libgui \
                          libcutils \
                          libui

ifeq ($(shell test $(ANDROID_MAJOR) -le 11 && echo true),true)
LOCAL_C_INCLUDES += frameworks/av/media/libmediaplayerservice
LOCAL_SHARED_LIBRARIES += libmediaplayerservice
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),5))
LOCAL_C_INCLUDES += frameworks/av/services/audiopolicy
else
LOCAL_C_INCLUDES += frameworks/av/services/audiopolicy \
                    frameworks/av/services/audiopolicy/common/include \
                    frameworks/av/services/audiopolicy/common/managerdefinitions/include \
                    frameworks/av/services/audiopolicy/engine/interface \
                    frameworks/av/services/audiopolicy/managerdefault \
                    frameworks/av/services/audiopolicy/service
endif

# libmedia was split into libmedia and libaudioclient starting with A8
ifeq ($(shell test $(ANDROID_MAJOR) -ge 8 && echo true),true)
LOCAL_SHARED_LIBRARIES += libaudioclient
else
LOCAL_SHARED_LIBRARIES += libmedia
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),8 9))
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.4
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),10))
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.5
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),11))
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.6
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),12 13))
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.7
LOCAL_C_INCLUDES += frameworks/native/libs/binder/include_activitymanager \
                    frameworks/native/libs/binder/include_batterystats \
                    frameworks/native/libs/binder/include_processinfo
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 12 && echo true),true)
LOCAL_C_INCLUDES += frameworks/av/media/libaudiohal/include \
                    frameworks/av/media/libheadtracking/include \
                    external/eigen
LOCAL_SHARED_LIBRARIES += libactivitymanager_aidl \
                          libbatterystats_aidl \
                          libmediautils \
                          libpermission \
                          audiopolicy-aidl-cpp
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),13))
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider-V1-ndk
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 10 && echo true),true)
LOCAL_SHARED_LIBRARIES += android.hardware.camera.device@3.4 \
                          libsensorprivacy
LOCAL_AIDL_INCLUDES := frameworks/native/libs/sensorprivacy/aidl
endif
ifeq ($(shell test $(ANDROID_MAJOR) -ge 12 && echo true),true)
LOCAL_SHARED_LIBRARIES += android.hardware.camera.device@3.7
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),13))
LOCAL_SHARED_LIBRARIES += android.hardware.camera.device-V1-ndk
LOCAL_AIDL_INCLUDES += hardware/interfaces/camera/device/aidl \
                       hardware/interfaces/camera/provider/aidl
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 8 && echo true),true)
LOCAL_C_INCLUDES += frameworks/native/libs/sensor/include \
                    frameworks/av/media/libstagefright/omx/include
LOCAL_SHARED_LIBRARIES += liblog \
                          libhidlbase \
                          libsensor \
                          android.frameworks.sensorservice@1.0 \
                          android.hardware.camera.common@1.0

ifeq ($(shell test $(ANDROID_MAJOR) -le 9 && echo true),true)
LOCAL_SHARED_LIBRARIES += libhidltransport \
                          libhwbinder
endif
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 9 && echo true),true)
LOCAL_SHARED_LIBRARIES += android.hidl.memory@1.0
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 11 && echo true),true)
LOCAL_SHARED_LIBRARIES += libmedia_codeclist \
                          libbinder_ndk
ifeq ($(shell test $(ANDROID_MAJOR) -le 11 && echo true),true)
LOCAL_SHARED_LIBRARIES += libresourcemanagerservice
endif
endif

LOCAL_MODULE_TAGS := optional
LOCAL_CPPFLAGS=-DANDROID_MAJOR=$(ANDROID_MAJOR) -DANDROID_MINOR=$(ANDROID_MINOR) -DANDROID_MICRO=$(ANDROID_MICRO) -Wno-unused-parameter
ifeq ($(MINIMEDIA_SENSORSERVER_DISABLE),1)
    LOCAL_CPPFLAGS += -DSENSORSERVER_DISABLE
endif
ifeq ($(AUDIOPOLICY_MTK_AUDIO_ADD),1)
LOCAL_CPPFLAGS += -DAUDIOPOLICY_MTK_AUDIO_ADD
endif
ifeq ($(shell grep -q listAudioSessions frameworks/av/services/audiopolicy/service/AudioPolicyService.h && echo true),true)
LOCAL_CPPFLAGS += -DAUDIOPOLICY_LINEAGE_AUDIOSESSIONINFO
endif
LOCAL_MODULE := minimediaservice
ifeq ($(shell test $(ANDROID_MAJOR) -le 11 && echo true),true)
ifeq ($(strip $(DROIDMEDIA_32)), true)
LOCAL_32_BIT_ONLY := true
endif
endif
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := minisf.cpp allocator.cpp
LOCAL_SHARED_LIBRARIES := libutils \
                          libbinder \
                          libmedia \
                          libgui \
                          libcutils \
                          libui

ifeq ($(shell test $(ANDROID_MAJOR) -ge 8 && echo true),true)
LOCAL_C_INCLUDES := frameworks/native/libs/sensor/include \
                    frameworks/native/include
LOCAL_SHARED_LIBRARIES += liblog \
                          libhidlbase \
                          libsensor \
                          android.frameworks.sensorservice@1.0 \
                          android.hardware.camera.common@1.0 \
                          android.hardware.camera.provider@2.4

ifeq ($(shell test $(ANDROID_MAJOR) -le 9 && echo true),true)
LOCAL_SHARED_LIBRARIES += libhidltransport \
                          libhwbinder
endif
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),9))
LOCAL_SHARED_LIBRARIES += android.hidl.memory@1.0
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 10 && echo true),true)
LOCAL_SHARED_LIBRARIES += libsensorprivacy
LOCAL_AIDL_INCLUDES := frameworks/native/libs/sensorprivacy/aidl
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 12 && echo true),true)
LOCAL_C_INCLUDES += frameworks/native/libs/binder/include_activitymanager \
                    frameworks/native/libs/binder/include_batterystats \
                    frameworks/native/libs/binder/include_processinfo
endif

LOCAL_MODULE_TAGS := optional
LOCAL_CPPFLAGS := -DANDROID_MAJOR=$(ANDROID_MAJOR) -DANDROID_MINOR=$(ANDROID_MINOR) -DANDROID_MICRO=$(ANDROID_MICRO) -Wno-unused-parameter
ifneq ($(CM_BUILD),)
LOCAL_CPPFLAGS += -DCM_BUILD
endif
ifneq ($(shell cat frameworks/native/services/surfaceflinger/SurfaceFlinger.h |grep getDisplayInfoEx),)
LOCAL_CPPFLAGS += -DUSE_SERVICES_VENDOR_EXTENSION
endif
LOCAL_MODULE := minisfservice
ifeq ($(shell test $(ANDROID_MAJOR) -le 11 && echo true),true)
ifeq ($(strip $(DROIDMEDIA_32)), true)
LOCAL_32_BIT_ONLY := true
endif
endif
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := libminisf.cpp allocator.cpp
LOCAL_SHARED_LIBRARIES := libutils \
                          libbinder \
                          libmedia \
                          libgui \
                          libcutils \
                          libui
LOCAL_MODULE_TAGS := optional
LOCAL_CPPFLAGS := -DANDROID_MAJOR=$(ANDROID_MAJOR) -DANDROID_MINOR=$(ANDROID_MINOR) -DANDROID_MICRO=$(ANDROID_MICRO) -Wno-unused-parameter
ifneq ($(CM_BUILD),)
LOCAL_CPPFLAGS += -DCM_BUILD
endif
ifneq ($(shell cat frameworks/native/services/surfaceflinger/SurfaceFlinger.h |grep getDisplayInfoEx),)
LOCAL_CPPFLAGS += -DUSE_SERVICES_VENDOR_EXTENSION
endif
ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),8 9))
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.4
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),10))
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.5
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),11))
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.6
endif

ifeq ($(ANDROID_MAJOR),$(filter $(ANDROID_MAJOR),12 13))
LOCAL_SHARED_LIBRARIES += android.hardware.camera.provider@2.7
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 12 && echo true),true)
LOCAL_SHARED_LIBRARIES += libactivitymanager_aidl \
                          libbatterystats_aidl \
                          libmediautils \
                          libpermission
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 10 && echo true),true)
LOCAL_SHARED_LIBRARIES += android.hardware.camera.device@3.4 \
                          libsensorprivacy
LOCAL_AIDL_INCLUDES := frameworks/native/libs/sensorprivacy/aidl
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 8 && echo true),true)
LOCAL_SHARED_LIBRARIES += liblog \
                          libcamera_client \
                          libhidlbase \
                          libsensor \
                          android.frameworks.sensorservice@1.0 \
                          android.hardware.camera.common@1.0

ifeq ($(shell test $(ANDROID_MAJOR) -le 9 && echo true),true)
LOCAL_SHARED_LIBRARIES += libhidltransport \
                          libhwbinder
endif
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 9 && echo true),true)
LOCAL_SHARED_LIBRARIES += android.hidl.memory@1.0
endif

ifeq ($(shell test $(ANDROID_MAJOR) -ge 12 && echo true),true)
LOCAL_C_INCLUDES += frameworks/native/libs/binder/include_activitymanager \
                    frameworks/native/libs/binder/include_batterystats \
                    frameworks/native/libs/binder/include_processinfo
endif

LOCAL_MODULE := libminisf
include $(BUILD_SHARED_LIBRARY)
