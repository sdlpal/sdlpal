# General makefile for generic unix & linux platforms

TARGET = sdlpal
TEST_TARGET = ./sdlpal-tests

HOST =

CFILES = $(wildcard ../adplug/*.c) $(wildcard ../libmad/*.c) $(wildcard ../liboggvorbis/src/*.c) $(wildcard ../*.c)
CFILES := $(filter-out ../main.c, $(CFILES))
CPPFILES = $(wildcard ../adplug/*.cpp) $(wildcard ../*.cpp) $(wildcard ./*.cpp)
OBJFILES = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

TEST_CPPFILES = $(wildcard ../tests/*.cpp)
TEST_OBJFILES = $(TEST_CPPFILES:.cpp=.o)

GTEST_DIR = ../3rd/googletest/googletest
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h

GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

TEST_CPPFLAGS += -isystem $(GTEST_DIR)/include
TEST_CXXFLAGS += -g -Wall -Wextra -pthread

CCFLAGS = `sdl2-config --cflags` -g -Wall -O2 -fno-strict-aliasing -I. -I../ -I../liboggvorbis/include -I../liboggvorbis/src -DPAL_HAS_PLATFORM_SPECIFIC_UTILS -I$(GTEST_DIR)
CXXFLAGS = $(CCFLAGS) -std=c++11 `fltk-config --cxxflags` $(TEST_CPPFLAGS) $(TEST_CXXFLAGS)
CFLAGS = $(CCFLAGS) -std=gnu99 `fltk-config --cflags`
LDFLAGS = `sdl2-config --libs` `fltk-config --ldflags` -lstdc++ -lm

all: $(TARGET)

gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

$(TARGET): $(OBJFILES) ../main.o
	$(HOST)g++ $^ -o $@ $(LDFLAGS)

$(TEST_TARGET): $(OBJFILES) $(TEST_OBJFILES) main-test.o gtest-all.o
	$(HOST)g++ $^ -o $@ $(LDFLAGS)

%.o: %.c
	$(HOST)gcc $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(HOST)g++ $(CXXFLAGS) -c $< -o $@

main-test.o: ../main.c
	$(HOST)gcc $(CFLAGS) -DUNIT_TEST=1 -c $< -o $@

clean:
	-rm -f $(TARGET) $(TEST_TARGET) *.o ../*.o ../adplug/*.o ../libmad/*.o ../liboggvorbis/src/*.o

check: $(TEST_TARGET)
	chmod +x $(TEST_TARGET)
	exec $(TEST_TARGET)
