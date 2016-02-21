# Adapted from Makefile for Dingux by palxex

TARGET = sdlpal.html

HOST =


CFILES = $(wildcard ../adplug/*.c) $(wildcard ../libmad/*.c) $(wildcard ../liboggvorbis/src/*.c) $(wildcard ../*.c) em.c
CPPFILES = $(wildcard ../adplug/*.cpp) $(wildcard ../*.cpp) $(wildcard ./*.cpp)
OBJFILES = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)

CFLAGS = -I${EMSCRIPTEN_TOOLCHAIN}/include/SDL2 -g -Wall -O2 -fno-strict-aliasing -I../ -I../liboggvorbis/include -I../liboggvorbis/src -DPAL_HAS_PLATFORM_SPECIFIC_UTILS
LDFLAGS = -L${EMSCRIPTEN_TOOLCHAIN}/lib -lSDL2 -lm -lstdc++ --preload-file data -s TOTAL_MEMORY=134217728 -s TOTAL_STACK=5242880 --emrun -s DEMANGLE_SUPPORT=1 -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 -s 'EMTERPRETIFY_FILE="sdlpal.bc"'

.c.o:
	emcc -c $(CFLAGS) $(CPPFLAGS) -o $@ $<
.cpp.o:
	em++ -c $(CFLAGS) $(CPPFLAGS) -o $@ $<
	
$(TARGET):$(OBJFILES)
	em++ $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(OBJFILES)
	rm -f $(TARGET) *.bc *.mem *.js *.data *.wast *.wasm

repackage:
	rm -f $(TARGET) *.bc *.mem *.js *.data *.wast *.wasm
	em++ $(CFLAGS) -o $(TARGET) $(OBJFILES) $(LDFLAGS)
