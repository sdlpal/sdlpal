# Makefile for win32 platforms with mingw

TARGET = sdlpal.exe
TEST_TARGET = ./sdlpal-tests

HOST =
TEST_CCFLAGS =

GTEST_DIR = ../3rd/googletest/googletest

CFILES = $(wildcard ../adplug/*.c) $(wildcard ../libmad/*.c) $(wildcard ../liboggvorbis/src/*.c) $(wildcard ../*.c) ../native_midi/native_midi_win32.c ../native_midi/native_midi_common.c
CPPFILES = $(wildcard ../adplug/*.cpp) $(wildcard ../*.cpp) $(wildcard ./*.cpp)
RCFILES = $(wildcard ./*.rc)
OBJFILES = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) $(RCFILES:.rc=.o)
TEST_CPPFILES = $(wildcard ../tests/*.cpp)
TEST_OBJFILES = $(TEST_CPPFILES:.cpp=.o)

override CCFLAGS += `sdl2-config --cflags` -g -msse -Wall -O2 -fno-strict-aliasing -I. -I../ -I../liboggvorbis/include -I../liboggvorbis/src -DPAL_HAS_PLATFORM_SPECIFIC_UTILS $(TEST_CCFLAGS)
CXXFLAGS = $(CCFLAGS) -std=c++11
CFLAGS = $(CCFLAGS) -std=gnu99
LDFLAGS = `sdl2-config --libs` -lm -lwinmm -lole32 -loleaut32 -limm32 -lcomctl32 -luuid -ldxguid -lversion -static -static-libgcc -static-libstdc++
TEST_CXXFLAGS += -isystem $(GTEST_DIR)/include -I $(GTEST_DIR) -g -Wall -Wextra -pthread

.PHONY : all clean check

all: $(TARGET)

$(TARGET): $(OBJFILES)
	@echo [LD] $@
	@$(HOST)g++ $^ -o $@ $(LDFLAGS)

gtest-all.o : $(GTEST_DIR)/src/gtest-all.cc
	@echo [CC] $^
	@$(HOST)g++ $(TEST_CXXFLAGS) -c $^ -o $@

%.o: %.c
	@echo [CC] $^
	@$(HOST)gcc $(CFLAGS) -c $^ -o $@

%.o: %.cpp
	@echo [CC] $^
	@$(HOST)g++ $(CXXFLAGS) -c $^ -o $@

%.o: %.rc
	@echo [RES] $^
	@$(HOST)windres -i $^ -o $@

$(TEST_TARGET): $(OBJFILES) $(TEST_OBJFILES) gtest-all.o
	@echo [LD] $@
	@$(HOST)g++ $^ -o $@ $(LDFLAGS) -lpthread

clean:
	-rm -f $(TARGET) $(TEST_TARGET) $(OBJFILES) $(TEST_OBJFILES)

check: TEST_CCFLAGS = -DUNIT_TEST=1 -isystem $(GTEST_DIR)/include
check: $(TEST_TARGET)
	@echo [EXEC] $(TEST_TARGET)
	@chmod +x $(TEST_TARGET)
	@exec $(TEST_TARGET)
