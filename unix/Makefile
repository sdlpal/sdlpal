# General makefile for generic unix & linux platforms

TARGET = sdlpal
TEST_TARGET = ./sdlpal-tests

HOST =
TEST_CCFLAGS =

GTEST_DIR = ../3rd/googletest/googletest

CFILES = $(wildcard ../adplug/*.c) $(wildcard ../libmad/*.c) $(wildcard ../liboggvorbis/src/*.c) $(wildcard ../*.c)
CPPFILES = $(wildcard ../adplug/*.cpp) $(wildcard ../*.cpp) $(wildcard ./*.cpp)
OBJFILES = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
TEST_CPPFILES = $(wildcard ../tests/*.cpp)
TEST_OBJFILES = $(TEST_CPPFILES:.cpp=.o)

CCFLAGS = `sdl2-config --cflags` -g -Wall -O2 -fno-strict-aliasing -I. -I../ -I../liboggvorbis/include -I../liboggvorbis/src -DPAL_HAS_PLATFORM_SPECIFIC_UTILS $(TEST_CCFLAGS)
CXXFLAGS = $(CCFLAGS) -std=c++11 `fltk-config --cxxflags`
CFLAGS = $(CCFLAGS) -std=gnu99 `fltk-config --cflags`
LDFLAGS = `sdl2-config --libs` `fltk-config --ldflags` -lstdc++ -lm -pthread
TEST_CXXFLAGS += -isystem $(GTEST_DIR)/include -I $(GTEST_DIR) -g -Wall -Wextra -pthread

.PHONY : all clean check

all: $(TARGET)

$(TARGET): $(OBJFILES)
	@echo [LD] $@
	@$(HOST)gcc $^ -o $@ $(LDFLAGS)

gtest-all.o : $(GTEST_DIR)/src/gtest-all.cc
	@echo [CC] $^
	@$(HOST)g++ $(TEST_CXXFLAGS) -c $< -o $@

%.o: %.c
	@echo [CC] $^
	@$(HOST)gcc $(CFLAGS) -c $< -o $@

%.o: %.cpp
	@echo [CC] $^
	@$(HOST)g++ $(CXXFLAGS) -c $< -o $@

$(TEST_TARGET): $(OBJFILES) $(TEST_OBJFILES) gtest-all.o
	@echo [LD] $@
	@$(HOST)g++ $^ -o $@ $(LDFLAGS) -lpthread

clean:
	-rm -f $(TARGET) $(TEST_TARGET) *.o ../*.o ../adplug/*.o ../libmad/*.o ../liboggvorbis/src/*.o ../tests/*.o

check: TEST_CCFLAGS = -DUNIT_TEST=1 -isystem $(GTEST_DIR)/include
check: $(TEST_TARGET)
	@echo [EXEC] $(TEST_TARGET)
	@chmod +x $(TEST_TARGET)
	@exec $(TEST_TARGET)
