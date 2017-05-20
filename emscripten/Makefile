# Adapted from Makefile for Dingux by palxex
# note: only works in chrome now, and audio not works now.

TARGET = sdlpal.html
EMFILES = sdlpal.{html,js,mem,data,bc,wast,wasm}

HOST =

DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td

CFILES = $(wildcard ../adplug/*.c) $(wildcard ../libmad/*.c) $(wildcard ../liboggvorbis/src/*.c) $(wildcard ../*.c) em.c
CPPFILES = $(wildcard ../adplug/*.cpp) $(wildcard ../*.cpp) $(wildcard ./*.cpp)
OBJFILES = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
DEPFILES = $(OBJFILES:.o=.d)

CFLAGS = -I${EMSCRIPTEN_TOOLCHAIN}/include/SDL2 -g -Wall -Oz -fno-strict-aliasing -I. -I../ -I../liboggvorbis/include -I../liboggvorbis/src -DPAL_HAS_PLATFORM_SPECIFIC_UTILS $(DEPFLAGS) 
CPPFLAGS = $(CFLAGS)
override LDFLAGS += -Oz -L${EMSCRIPTEN_TOOLCHAIN}/lib -lSDL2 -lm -lstdc++ --preload-file data -s TOTAL_MEMORY=134217728 -s TOTAL_STACK=5242880 --emrun -s DEMANGLE_SUPPORT=1 -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 -s 'EMTERPRETIFY_FILE="sdlpal.bc"' -s WASM=1

POSTCOMPILE = @mv -f $*.Td $*.d && touch $@

%.o: %.c %.d
	@echo [CC] $<
	@emcc -c $(CFLAGS) -o $@ $<
	$(POSTCOMPILE)

%.o: %.cpp %.d
	@echo [CC] $<
	@em++ -c $(CPPFLAGS) -o $@ $<
	$(POSTCOMPILE)
	
$(TARGET):$(OBJFILES)
	@echo [LD] $@
	@em++ -o $(TARGET) $(OBJFILES) $(LDFLAGS)

clean:
	rm -f $(TARGET) $(EMFILES) $(OBJFILES) $(DEPFILES)

repackage:
	rm -f $(TARGET) $(EMFILES)
	em++ -o $(TARGET) $(OBJFILES) $(LDFLAGS)

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)
