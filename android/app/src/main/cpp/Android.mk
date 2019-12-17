NDK_LOCAL_PATH := $(call my-dir)
SDLPAL_PATH := $(NDK_LOCAL_PATH)/../../../../..
SDL_PATH := $(SDLPAL_PATH)/3rd/SDL

include $(SDL_PATH)/Android.mk

include $(CLEAR_VARS)
GENERATED := -DPAL_HAS_GIT_REVISION -DHAVE_CONFIG_H $(shell $(SDLPAL_PATH)/scripts/gengitrev)
LOCAL_PATH := $(NDK_LOCAL_PATH)
LOCAL_MODULE := main

OGG_PATH := $(SDLPAL_PATH)/liboggvorbis
OPUS_PATH := $(SDLPAL_PATH)/libopusfile

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(SDLPAL_PATH) $(SDL_PATH)/include $(OGG_PATH)/include $(OGG_PATH)/src $(OPUS_PATH)/include $(OPUS_PATH)/src $(OPUS_PATH)/celt $(OPUS_PATH)/silk $(OPUS_PATH)/silk/float

LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
    $(wildcard $(SDLPAL_PATH)/*.cpp) $(wildcard $(SDLPAL_PATH)/*.c) \
    $(wildcard $(SDLPAL_PATH)/adplug/*.c) $(wildcard $(SDLPAL_PATH)/adplug/*.cpp) \
    $(wildcard $(SDLPAL_PATH)/libopusfile/src/*.c) \
    $(wildcard $(SDLPAL_PATH)/libopusfile/celt/*.c) \
    $(wildcard $(SDLPAL_PATH)/libopusfile/silk/*.c) \
    $(wildcard $(SDLPAL_PATH)/libopusfile/silk/float/*.c) \
    $(wildcard $(SDLPAL_PATH)/liboggvorbis/src/*.c) \
    $(wildcard $(SDLPAL_PATH)/libmad/*.c) \
    $(wildcard $(SDLPAL_PATH)/native_midi/*.c) \
    $(wildcard $(LOCAL_PATH)/*.cpp) \
    $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_CFLAGS += -std=gnu99 -DPAL_HAS_PLATFORM_SPECIFIC_UTILS $(GENERATED)

LOCAL_CPPFLAGS += -std=c++11 -DPAL_HAS_PLATFORM_SPECIFIC_UTILS $(GENERATED)

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
