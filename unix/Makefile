# General makefile for generic unix & linux platforms

TARGET = sdlpal

HOST =

CFILES = $(wildcard ../adplug/*.c) $(wildcard ../libmad/*.c) $(wildcard ../liboggvorbis/src/*.c) $(wildcard ../*.c)
CPPFILES = $(wildcard ../adplug/*.cpp) $(wildcard ../*.cpp) $(wildcard ./*.cpp)
OBJFILES = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

CCFLAGS = `sdl2-config --cflags` -g -Wall -O2 -fno-strict-aliasing -I../ -I../liboggvorbis/include -I../liboggvorbis/src -DPAL_HAS_PLATFORM_SPECIFIC_UTILS
CXXFLAGS = $(CCFLAGS) -std=c++11 `fltk-config --cxxflags`
CFLAGS = $(CCFLAGS) -std=gnu99 `fltk-config --cflags`
LDFLAGS = `sdl2-config --libs` `fltk-config --ldflags` -lstdc++ -lm

$(TARGET): $(OBJFILES)
	$(HOST)g++ $(OBJFILES) -o $(TARGET) $(LDFLAGS)

%.o: %.c
	$(HOST)gcc $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(HOST)gcc $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET)
