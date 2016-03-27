LOCAL_PATH:= $(call my-dir)

ANDROID_MAJOR :=
ANDROID_MINOR :=
ANDROID_MICRO :=

ifndef ANDROID_MAJOR
include build/core/version_defaults.mk
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

ifeq ($(strip $(ANDROID_MICRO)),)
$(warning *** ANDROID_MICRO undefined. Assuming 0)
ANDROID_MICRO = 0
endif

include $(CLEAR_VARS)
LOCAL_SRC_FILES := droidmedia.cpp \
                   droidmediacamera.cpp \
                   droidmediaconstants.cpp \
                   droidmediacodec.cpp \
                   droidmediaconvert.cpp \
                   allocator.cpp \
                   droidmediabuffer.cpp \
                   private.cpp

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
LOCAL_CPPFLAGS=-DANDROID_MAJOR=$(ANDROID_MAJOR) -DANDROID_MINOR=$(ANDROID_MINOR) -DANDROID_MICRO=$(ANDROID_MICRO)
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libdroidmedia
LOCAL_C_INCLUDES := frameworks/native/include/media/openmax
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := minimedia.cpp
LOCAL_C_INCLUDES := frameworks/av/services/camera/libcameraservice \
                    frameworks/av/media/libmediaplayerservice \
                    frameworks/av/services/audiopolicy \
                    system/media/camera/include
LOCAL_SHARED_LIBRARIES := libcameraservice \
                          libmediaplayerservice \
                          libaudiopolicyservice \
                          libutils \
                          libbinder \
                          libgui \
                          libcutils \
                          libui
LOCAL_MODULE_TAGS := optional
LOCAL_CPPFLAGS=-DANDROID_MAJOR=$(ANDROID_MAJOR) -DANDROID_MINOR=$(ANDROID_MINOR) -DANDROID_MICRO=$(ANDROID_MICRO)
LOCAL_MODULE := minimediaservice
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := minisf.cpp allocator.cpp
LOCAL_SHARED_LIBRARIES := libutils \
                          libbinder \
                          libgui \
                          libcutils \
                          libui
LOCAL_MODULE_TAGS := optional
LOCAL_CPPFLAGS := -DANDROID_MAJOR=$(ANDROID_MAJOR) -DANDROID_MINOR=$(ANDROID_MINOR) -DANDROID_MICRO=$(ANDROID_MICRO)
ifneq ($(CM_BUILD),)
LOCAL_CPPFLAGS += -DCM_BUILD
endif
LOCAL_MODULE := minisfservice
include $(BUILD_EXECUTABLE)
