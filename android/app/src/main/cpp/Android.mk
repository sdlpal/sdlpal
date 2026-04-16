NDK_LOCAL_PATH := $(call my-dir)
SDLPAL_PATH := $(NDK_LOCAL_PATH)/../../../../..
SDL_PATH := $(SDLPAL_PATH)/3rd/SDL

include $(SDL_PATH)/Android.mk

include $(CLEAR_VARS)
GENERATED := -DPAL_HAS_GIT_REVISION $(shell $(SDLPAL_PATH)/scripts/gengitrev)
LOCAL_PATH := $(NDK_LOCAL_PATH)
LOCAL_MODULE := main

TIMIDITY_PATH := $(SDLPAL_PATH)/timidity

LOCAL_C_INCLUDES := $(LOCAL_PATH) $(SDLPAL_PATH) $(SDLPAL_PATH)/sdl_compat $(SDL_PATH)/include $(SDLPAL_PATH)/3rd/compat $(TIMIDITY_PATH)

LOCAL_SRC_FILES := $(SDLPAL_PATH)/sdl_compat/sdl_compat.c \
    $(wildcard $(SDLPAL_PATH)/*.cpp) $(wildcard $(SDLPAL_PATH)/*.c) \
    $(wildcard $(SDLPAL_PATH)/adplug/*.c) $(wildcard $(SDLPAL_PATH)/adplug/*.cpp) \
    $(wildcard $(SDLPAL_PATH)/native_midi/*.c) \
    $(wildcard $(SDLPAL_PATH)/timidity/*.c) \
    $(wildcard $(LOCAL_PATH)/*.cpp) \
    $(wildcard $(LOCAL_PATH)/*.c)

LOCAL_CFLAGS += -std=gnu99 -DPAL_HAS_PLATFORM_SPECIFIC_UTILS $(GENERATED) -DUSE_SDL3=1 -Daccess=SAF_access -Dfopen=SAF_fopen

LOCAL_CPPFLAGS += -std=c++11 -DPAL_HAS_PLATFORM_SPECIFIC_UTILS $(GENERATED)

LOCAL_SHARED_LIBRARIES := SDL3

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
