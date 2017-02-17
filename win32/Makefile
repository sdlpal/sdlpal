# Makefile for win32 platforms with mingw

TARGET = sdlpal.exe

HOST =

CFILES = $(wildcard ../adplug/*.c) $(wildcard ../libmad/*.c) $(wildcard ../liboggvorbis/src/*.c) $(wildcard ../*.c) ../native_midi/native_midi_win32.c ../native_midi/native_midi_common.c
CPPFILES = $(wildcard ../adplug/*.cpp) $(wildcard ../*.cpp) $(wildcard ./*.cpp)
OBJFILES = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o) sdlpal.o

override CCFLAGS += `sdl2-config --cflags` -msse -Wall -O2 -fno-strict-aliasing -I../ -I../liboggvorbis/include -I../liboggvorbis/src -DPAL_HAS_PLATFORM_SPECIFIC_UTILS
CXXFLAGS = $(CCFLAGS) -std=c++11
CFLAGS = $(CCFLAGS) -std=gnu99
LDFLAGS = `sdl2-config --libs` -lm -lwinmm -lole32 -loleaut32 -limm32 -lcomctl32 -luuid -ldxguid -lversion -static -static-libgcc -static-libstdc++

$(TARGET): $(OBJFILES)
	$(HOST)g++ $(OBJFILES) -o $(TARGET) $(LDFLAGS)
	$(HOST)strip $(TARGET)

%.o: %.c
	$(HOST)gcc $(CFLAGS) -c $< -o $@

%.o: %.cpp
	$(HOST)g++ $(CXXFLAGS) -c $< -o $@
	
%.o: %.rc
	windres -i $< -o $@

clean:
	-rm -f $(TARGET) *.o ../*.o ../adplug/*.o ../libmad/*.o ../liboggvorbis/src/*.o
