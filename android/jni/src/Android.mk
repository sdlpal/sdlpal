CURRENT_PATH := $(call my-dir)

SDL_PATH := $(CURRENT_PATH)/../../../SDL2
include $(SDL_PATH)/Android.mk

include $(CLEAR_VARS)
LOCAL_PATH := $(CURRENT_PATH)/../../..
LOCAL_MODULE := main

LOCAL_C_INCLUDES := $(CURRENT_PATH) $(LOCAL_PATH) $(SDL_PATH)/include $(LOCAL_PATH)/src/liboggvorbis/include $(LOCAL_PATH)/src/liboggvorbis/src

LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
    $(wildcard $(LOCAL_PATH)/src/*.cpp) $(wildcard $(LOCAL_PATH)/src/*.c) \
    $(wildcard $(LOCAL_PATH)/src/adplug/*.c) $(wildcard $(LOCAL_PATH)/src/adplug/*.cpp) \
    $(wildcard $(LOCAL_PATH)/src/liboggvorbis/src/*.c) \
    $(wildcard $(LOCAL_PATH)/src/libmad/*.c)

LOCAL_CFLAGS += -std=c99

LOCAL_CPPFLAGS += -std=c++11

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
