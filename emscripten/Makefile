# Adapted from Makefile for Dingux by palxex
# note: only works in chrome now, and audio not works now.

TARGET = sdlpal.html
EMFILES = sdlpal.{html,js,mem,data,bc,wast,wasm}

INTER=.emscripten

HOST =

DEPFLAGS = -MT $@ -MMD -MP -MF $*$(INTER).Td

CFILES = $(wildcard ../adplug/*.c) $(wildcard ../libmad/*.c) $(wildcard ../liboggvorbis/src/*.c) $(wildcard ../*.c)
CPPFILES = $(wildcard ../adplug/*.cpp) $(wildcard ../*.cpp) $(wildcard ./*.cpp)
OBJFILES = $(CFILES:.c=$(INTER).o) $(CPPFILES:.cpp=$(INTER).o)
DEPFILES = $(OBJFILES:.o=.d)

CFLAGS = -I${EMSCRIPTEN_TOOLCHAIN}/include/SDL2 -g -Wall -O2 -fno-strict-aliasing -I. -I../ -I../liboggvorbis/include -I../liboggvorbis/src -DPAL_HAS_PLATFORM_SPECIFIC_UTILS $(DEPFLAGS) 
CPPFLAGS = $(CFLAGS) -std=c++11
override LDFLAGS += -O2 -L${EMSCRIPTEN_TOOLCHAIN}/lib -lGL -lSDL2 -lm -lstdc++ --preload-file data -s TOTAL_MEMORY=134217728 -s TOTAL_STACK=5242880 --emrun -s DEMANGLE_SUPPORT=1 -s EMTERPRETIFY=1 -s EMTERPRETIFY_ASYNC=1 -s 'EMTERPRETIFY_FILE="sdlpal.bc"' -s WASM=1 -s NO_EXIT_RUNTIME=1  -s 'EXTRA_EXPORTED_RUNTIME_METHODS=["Pointer_stringify"]' -s ASSERTIONS=1

POSTCOMPILE = @mv -f $*$(INTER).Td $*$(INTER).d && touch $@

%$(INTER).o: %.c %$(INTER).d
	@echo [CC] $<
	@emcc -c $(CFLAGS) -o $@ $<
	$(POSTCOMPILE)

%$(INTER).o: %.cpp %$(INTER).d
	@echo [CC] $<
	@em++ -c $(CPPFLAGS) -o $@ $<
	$(POSTCOMPILE)
	
$(TARGET):$(OBJFILES)
	@echo [LD] $@
	@em++ -o $(TARGET) $(OBJFILES) $(LDFLAGS)
	sh process.sh

exec:$(TARGET)
	emrun $(TARGET)

clean:
	rm -f $(TARGET) $(EMFILES) $(OBJFILES) $(DEPFILES)

repackage:
	rm -f $(TARGET) $(EMFILES)
	em++ -o $(TARGET) $(OBJFILES) $(LDFLAGS)
	sh process.sh

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)
