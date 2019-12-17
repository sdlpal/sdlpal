# General makefile for generic unix & linux platforms

TARGET = sdlpal
TEST_TARGET = ./sdlpal-tests

INTER=.unix

HOST =
TEST_CCFLAGS =

GENERATED = -DPAL_HAS_GIT_REVISION $(shell ../scripts/gengitrev)

DEPFLAGS = -MT $@ -MMD -MP -MF $*$(INTER).Td

GTEST_DIR = ../3rd/googletest/googletest

USE_ALSA ?= $(shell echo "int main(){}" | gcc -o /dev/null -x c - -lasound 2>/dev/null && echo 1)
ifeq (${USE_ALSA},1)
	MIDI_CFILES = $(wildcard ../native_midi/*.c) native_midi_alsa.c
	MIDI_CPPFILES =
	MIDI_LDFLAGS = -lasound
else
	MIDI_CFILES =
	MIDI_CPPFILES = native_midi.cpp
	MIDI_LDFLAGS =
endif

SOURCES = .. ../adplug ../liboggvorbis/src ../libopusfile/src ../libopusfile/celt ../libopusfile/silk ../libopusfile/silk/float ../libmad
CFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c)) $(MIDI_CFILES)
CPPFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp)) unix.cpp $(MIDI_CPPFILES)
TOBJFILES = $(CFILES:.c=$(INTER).o) $(CPPFILES:.cpp=$(INTER).o)
DEPFILES = $(OBJFILES:.o=.d)
TEST_CPPFILES = $(wildcard ../tests/*.cpp)
TEST_OBJFILES = $(TEST_CPPFILES:.cpp=.o)
SDL_CONFIG = sdl2-config


FLTK_INSTALLED := $(shell command -v fltk-config 2> /dev/null)
ifdef FLTK_INSTALLED
	FLTK_CXXFLAGS = `fltk-config --cxxflags`
	FLTK_CFLAGS = `fltk-config --cflags`
	FLTK_LDFLAGS = `fltk-config --ldflags`
else
	FLTK_CXXFLAGS = -DPAL_NO_LAUNCH_UI
	FLTK_CFLAGS =
	FLTK_LDFLAGS =
endif


CC = $(HOST)gcc
CXX = $(HOST)g++
override CCFLAGS += `$(SDL_CONFIG) --cflags` -D_GNU_SOURCE -g -Wall -O2 -fno-strict-aliasing $(EXTRA_CCFLAGS) $(TEST_CCFLAGS) $(GENERATED)
CXXFLAGS = $(CCFLAGS) -std=c++11 $(FLTK_CXXFLAGS)
CFLAGS = $(CCFLAGS) -std=gnu99 $(FLTK_CFLAGS)
LDFLAGS = `$(SDL_CONFIG) --libs` $(FLTK_LDFLAGS) $(MIDI_LDFLAGS) -lstdc++ -lm
TEST_CXXFLAGS += -isystem $(GTEST_DIR)/include -I $(GTEST_DIR) -g -Wall -Wextra -pthread

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
	LDFLAGS += -framework OpenGL
	OBJFILES = $(filter-out ../mini_glloader.unix.o, $(TOBJFILES))
else
	LDFLAGS += -lGL -pthread
	OBJFILES = $(TOBJFILES)
endif

POSTCOMPILE = @mv -f $*$(INTER).Td $*$(INTER).d && touch $@

.PHONY : all clean check

all: $(TARGET)

$(TARGET): $(OBJFILES)
	@echo [LD] $@
	@$(CXX) $^ -o $@ $(LDFLAGS)

gtest-all.o : $(GTEST_DIR)/src/gtest-all.cc %.d
	@echo [CC] $<
	@$(CXX) $(DEPFLAGS) $(TEST_CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%$(INTER).o: %.c %$(INTER).d
	@echo [CC] $<
	@$(CC) $(DEPFLAGS) $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%$(INTER).o: %.cpp %$(INTER).d
	@echo [CC] $<
	@$(CXX) $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

$(TEST_TARGET): $(OBJFILES) $(TEST_OBJFILES) gtest-all.o
	@echo [LD] $@
	@$(CXX) $^ -o $@ $(LDFLAGS) -lpthread

clean:
	-@rm -f $(TARGET) $(TEST_TARGET) $(OBJFILES) $(TEST_OBJFILES) $(DEPFILES)

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)

../liboggvorbis/%$(INTER).o: EXTRA_CCFLAGS := -I../liboggvorbis/include -I../liboggvorbis/src
../libopusfile/%$(INTER).o: EXTRA_CCFLAGS := -I../liboggvorbis/include -I../libopusfile/include -I../libopusfile/src -I../libopusfile/celt -I../libopusfile/silk -I../libopusfile/silk/float -DHAVE_CONFIG_H
%$(INTER).o: EXTRA_CCFLAGS := -I. -I.. -I../liboggvorbis/include -I../libopusfile/include -DPAL_HAS_PLATFORM_SPECIFIC_UTILS
check: TEST_CCFLAGS = -I. -I.. -DUNIT_TEST=1 -isystem $(GTEST_DIR)/include
check: $(TEST_TARGET)
	@echo [EXEC] $(TEST_TARGET)
	@chmod +x $(TEST_TARGET)
	@exec $(TEST_TARGET)
