# Adapted from Makefile for Dingux by palxex
# note: only works in chrome now, and audio not works now.

TARGET = sdlpal.js
EMFILES = sdlpal.{js,mem,bc,wast,wasm,wasm.map}
EMPORTS = -s USE_SDL=2 -s USE_OGG=1 -s USE_VORBIS=1

INTER=.emscripten

HOST =

GENERATED = -DPAL_HAS_GIT_REVISION $(shell ../scripts/gengitrev)

DEPFLAGS = -MT $@ -MMD -MP -MF $*$(INTER).Td

CFILES = $(wildcard ../adplug/*.c) $(wildcard ../libmad/*.c) $(wildcard ../*.c)
CPPFILES = $(wildcard ../adplug/*.cpp) $(wildcard ../*.cpp) emscripten_adapter.cpp
OBJFILES = $(CFILES:.c=$(INTER).o) $(CPPFILES:.cpp=$(INTER).o)
DEPFILES = $(OBJFILES:.o=.d)

CFLAGS = -g4 -Wall -O2 -fno-strict-aliasing -I. -I../ -DPAL_HAS_PLATFORM_SPECIFIC_UTILS $(DEPFLAGS) $(EMPORTS) $(GENERATED)
CPPFLAGS = $(CFLAGS) -std=c++11
override LDFLAGS += -g4 -O2 $(EMPORTS) -s TOTAL_MEMORY=134217728 -s TOTAL_STACK=5242880 --emrun -lidbfs.js -s DEMANGLE_SUPPORT=1 -s ASYNCIFY=1 -s NO_EXIT_RUNTIME=1 \
	-s EXPORTED_FUNCTIONS="['_EMSCRIPTEN_main']" -s 'EXTRA_EXPORTED_RUNTIME_METHODS=["Pointer_stringify", "cwrap"]' -s USE_WEBGL2=1 -s ASSERTIONS=1 --source-map-base http://localhost:8080/

POSTCOMPILE = @mv -f $*$(INTER).Td $*$(INTER).d && touch $@

%$(INTER).o: %.c %$(INTER).d
	@echo [CC] $<
	@emcc -c $(CFLAGS) -Dfclose=EMSCRIPTEN_fclose -Dmain=EMSCRIPTEN_main -o $@ $<
	$(POSTCOMPILE)

%$(INTER).o: %.cpp %$(INTER).d
	@echo [CC] $<
	@em++ -c $(CPPFLAGS) -o $@ $<
	$(POSTCOMPILE)

$(TARGET):$(OBJFILES)
	@echo [LD] $@
	@em++ -o $(TARGET) $(OBJFILES) $(LDFLAGS)

exec:$(TARGET)
	emrun --no_browser --hostname 0.0.0.0 --port 8080 $(TARGET)

clean:
	rm -f $(TARGET) $(EMFILES) $(OBJFILES) $(DEPFILES)

repackage:
	rm -f $(TARGET) $(EMFILES)
	em++ -o $(TARGET) $(OBJFILES) $(LDFLAGS)

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)
