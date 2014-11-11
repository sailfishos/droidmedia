LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := droidmedia.cpp \
                   droidmediacamera.cpp \
                   allocator.cpp \
                   mediabuffer.cpp

LOCAL_SHARED_LIBRARIES := libc \
                          libutils \
                          libcutils \
                          libcamera_client \
                          libgui \
                          libui \
                          libbinder

LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := libdroidmedia
include $(BUILD_SHARED_LIBRARY)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := test.cpp
LOCAL_SHARED_LIBRARIES := libdroidmedia
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := test_droidmedia
include $(BUILD_EXECUTABLE)

include $(CLEAR_VARS)
LOCAL_SRC_FILES := miniservice.cpp
LOCAL_C_INCLUDES := frameworks/av/services/camera/libcameraservice
LOCAL_SHARED_LIBRARIES := libcameraservice \
                          libutils \
                          libbinder
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE := minicameraservice
include $(BUILD_EXECUTABLE)
