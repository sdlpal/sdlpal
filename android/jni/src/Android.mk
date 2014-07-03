LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../SDL2

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include

LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	../../../battle.c \
	../../../global.c \
	../../../map.c \
	../../../res.c \
	../../../ui.c \
	../../../ending.c \
	../../../input.c \
	../../../rngplay.c \
	../../../uibattle.c \
	../../../fight.c \
	../../../palcommon.c \
	../../../scene.c \
	../../../uigame.c \
	../../../font.c \
	../../../itemmenu.c \
	../../../palette.c \
	../../../script.c \
	../../../util.c \
	../../../game.c \
	../../../magicmenu.c \
	../../../play.c \
	../../../sound.c \
	../../../video.c \
	../../../getopt.c \
	../../../main.c \
	../../../private.c \
	../../../text.c \
	../../../yj1.c \
	../../../rixplay.cpp \
	../../../adplug/binfile.cpp \
	../../../adplug/binio.cpp \
	../../../adplug/dosbox_opl.cpp \
	../../../adplug/emuopl.cpp \
	../../../adplug/fmopl.c \
	../../../adplug/fprovide.cpp \
	../../../adplug/player.cpp \
	../../../adplug/rix.cpp \
	../../../adplug/surroundopl.cpp \
	../../../libmad/bit.c \
	../../../libmad/decoder.c \
	../../../libmad/fixed.c \
	../../../libmad/frame.c \
	../../../libmad/huffman.c \
	../../../libmad/layer12.c \
	../../../libmad/layer3.c \
	../../../libmad/music_mad.c \
	../../../libmad/stream.c \
	../../../libmad/synth.c \
	../../../libmad/timer.c

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
