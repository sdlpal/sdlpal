TARGET = sdlpal

SOURCES = . ../src ../src/adplug ../src/libmad
CFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c)) input_PSP.c main_PSP.c
CPPFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))
OFILES = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

PSPSDK = $(shell psp-config --pspsdk-path)

INCDIR = 
CFLAGS = -O3 -G0 -Wall -g `$(PSPSDK)/../bin/sdl-config --cflags` -DPSP=1 -I. -I../src
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti `$(PSPSDK)/../bin/sdl-config --cflags`
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = `$(PSPSDK)/../bin/sdl-config --libs` -lstdc++

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = sdlpal

BUILD_PRX = 1
PSP_FW_VERSION = 371

include $(PSPSDK)/lib/build.mak
