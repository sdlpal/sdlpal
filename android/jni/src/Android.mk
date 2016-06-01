LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../../../SDL2
OGG_PATH := ../../../liboggvorbis

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include $(LOCAL_PATH)/$(OGG_PATH)/include $(LOCAL_PATH)/$(OGG_PATH)/src

LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
    $(wildcard ../../*.cpp) $(wildcard ../../*.c) \
    $(wildcard ../../adplug/*.c) $(wildcard ../../adplug/*.cpp) \
    $(wildcard ../../liboggvorbis/src/*.c) \
    $(wildcard ../../libmad/*.c)

LOCAL_CFLAGS += -std=c99

LOCAL_CPPFLAGS += -std=c++11

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
