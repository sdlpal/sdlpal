LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := main

SDL_PATH := ../../../SDL2
OGG_PATH := ../../../liboggvorbis

LOCAL_C_INCLUDES := $(LOCAL_PATH)/$(SDL_PATH)/include $(LOCAL_PATH)/$(OGG_PATH)/include $(LOCAL_PATH)/$(OGG_PATH)/src

LOCAL_SRC_FILES := $(SDL_PATH)/src/main/android/SDL_android_main.c \
	../../../battle.c \
	../../../ending.c \
	../../../fight.c \
	../../../font.c \
	../../../game.c \
	../../../global.c \
	../../../input.c \
	../../../itemmenu.c \
	../../../magicmenu.c \
	../../../main.c \
	../../../map.c \
	../../../mp3play.c \
	../../../oggplay.c \
	../../../overlay.c \
	../../../palcommon.c \
	../../../palcfg.c \
	../../../palette.c \
	../../../play.c \
	../../../resampler.c \
	../../../res.c \
	../../../rixplay.cpp \
	../../../rngplay.c \
	../../../scene.c \
	../../../script.c \
	../../../sound.c \
	../../../text.c \
	../../../ui.c \
	../../../uibattle.c \
	../../../uigame.c \
	../../../util.c \
	../../../video.c \
	../../../yj1.c \
	../../../adplug/binfile.cpp \
	../../../adplug/binio.cpp \
	../../../adplug/dbemuopl.cpp \
	../../../adplug/dbopl.cpp \
	../../../adplug/demuopl.cpp \
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
	../../../libmad/timer.c \
	../../../liboggvorbis/src/analysis.c \
	../../../liboggvorbis/src/bitrate.c \
    ../../../liboggvorbis/src/bitwise.c \
	../../../liboggvorbis/src/block.c \
	../../../liboggvorbis/src/codebook.c \
    ../../../liboggvorbis/src/envelope.c \
	../../../liboggvorbis/src/floor0.c \
	../../../liboggvorbis/src/floor1.c \
    ../../../liboggvorbis/src/framing.c \
	../../../liboggvorbis/src/info.c \
	../../../liboggvorbis/src/lookup.c \
    ../../../liboggvorbis/src/lpc.c \
	../../../liboggvorbis/src/lsp.c \
	../../../liboggvorbis/src/mapping0.c \
    ../../../liboggvorbis/src/mdct.c \
	../../../liboggvorbis/src/psy.c \
	../../../liboggvorbis/src/registry.c \
    ../../../liboggvorbis/src/res0.c \
	../../../liboggvorbis/src/sharedbook.c \
	../../../liboggvorbis/src/smallft.c \
    ../../../liboggvorbis/src/synthesis.c \
	../../../liboggvorbis/src/vorbisenc.c \
	../../../liboggvorbis/src/vorbisfile.c \
    ../../../liboggvorbis/src/window.c

LOCAL_SHARED_LIBRARIES := SDL2

LOCAL_LDLIBS := -lGLESv1_CM -lGLESv2 -llog

include $(BUILD_SHARED_LIBRARY)
