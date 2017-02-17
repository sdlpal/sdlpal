SDLPAL_LOCAL_PATH := $(call my-dir)
SDL_PATH := $(SDLPAL_LOCAL_PATH)/../../../SDL2

include $(SDL_PATH)/Android.mk

include $(CLEAR_VARS)
LOCAL_PATH := $(SDLPAL_LOCAL_PATH)
LOCAL_MODULE := main

OGG_PATH := ../../../liboggvorbis

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include $(LOCAL_PATH)/$(OGG_PATH)/include $(LOCAL_PATH)/$(OGG_PATH)/src

LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
    $(wildcard $(LOCAL_PATH)/../../../*.cpp) $(wildcard $(LOCAL_PATH)/../../../*.c) \
    $(wildcard $(LOCAL_PATH)/../../../adplug/*.c) $(wildcard $(LOCAL_PATH)/../../../adplug/*.cpp) \
    $(wildcard $(LOCAL_PATH)/../../../liboggvorbis/src/*.c) \
    $(wildcard $(LOCAL_PATH)/../../../libmad/*.c)

LOCAL_CFLAGS += -std=c99

LOCAL_CPPFLAGS += -std=c++11

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
