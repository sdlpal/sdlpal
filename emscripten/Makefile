# Adapted from Makefile for Dingux by palxex
# note: only works in chrome now, and audio not works now.

TARGET = sdlpal.js
EMFILES = sdlpal.{js,mem,bc,wast,wasm,wasm.map}
EMPORTS = -s USE_OGG=1 -s USE_VORBIS=1

INTER=.emscripten

HOST =

GENERATED = -DPAL_HAS_GIT_REVISION $(shell ../scripts/gengitrev)

DEPFLAGS = -MT $@ -MMD -MP -MF $*$(INTER).Td

CFILES = $(wildcard ../sdl_compat/*.c) $(wildcard ../adplug/*.c) $(wildcard ../libmad/*.c) $(wildcard ../timidity/*.c) $(wildcard ../*.c)
CPPFILES = $(wildcard ../adplug/*.cpp) $(wildcard ../*.cpp) emscripten_adapter.cpp
OBJFILES = $(CFILES:.c=$(INTER).o) $(CPPFILES:.cpp=$(INTER).o)
DEPFILES = $(OBJFILES:.o=.d)
SDL_CONFIG = pkg-config sdl3

override CFLAGS = `$(SDL_CONFIG) --cflags` -gsource-map -Wall -O2 -fno-strict-aliasing -I. -I.. -DUSE_SDL3=1 -I../sdl_compat -I../timidity -DPAL_HAS_PLATFORM_SPECIFIC_UTILS $(DEPFLAGS) $(EMPORTS) $(GENERATED)
CPPFLAGS = $(CFLAGS) -std=c++11
override LDFLAGS += `$(SDL_CONFIG) --libs` -gsource-map -O2 $(EMPORTS) --emrun -lidbfs.js -s ASYNCIFY=1 -sEXIT_RUNTIME=1 \
	-s EXPORTED_FUNCTIONS="['_EMSCRIPTEN_main']" -s 'EXPORTED_RUNTIME_METHODS=["cwrap"]' -sMAX_WEBGL_VERSION=2 -sFULL_ES3 -s TOTAL_STACK=5242880

POSTCOMPILE = @mv -f $*$(INTER).Td $*$(INTER).d && touch $@

%$(INTER).o: %.c %$(INTER).d
	@echo [CC] $<
	@emcc -c $(CFLAGS) -Dfopen=EMSCRIPTEN_fopen -Dfclose=EMSCRIPTEN_fclose -Dmain=EMSCRIPTEN_main -o $@ $<
	$(POSTCOMPILE)

%$(INTER).o: %.cpp %$(INTER).d
	@echo [CC] $<
	@em++ -c $(CPPFLAGS) -o $@ $<
	$(POSTCOMPILE)

$(TARGET):$(OBJFILES)
	@echo [LD] $@
	@em++ -o $(TARGET) $(OBJFILES) $(LDFLAGS)

exec:$(TARGET)
	emrun sdlpal.html

clean:
	rm -f $(TARGET) $(EMFILES) $(OBJFILES) $(DEPFILES)

repackage:
	rm -f $(TARGET) $(EMFILES)
	em++ -o $(TARGET) $(OBJFILES) $(LDFLAGS)

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)
