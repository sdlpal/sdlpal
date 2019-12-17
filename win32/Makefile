# Makefile for win32 platforms with mingw under msys or linux

TARGET = sdlpal.exe
TEST_TARGET = sdlpal-tests.exe

INTER=.msys

HOST =
TEST_CCFLAGS =

GENERATED = -DPAL_HAS_GIT_REVISION $(shell ../scripts/gengitrev)

DEPFLAGS = -MT $@ -MMD -MP -MF $*$(INTER).Td

GTEST_DIR = ../3rd/googletest/googletest

SOURCES = . .. ../adplug ../liboggvorbis/src ../libopusfile/src ../libopusfile/celt ../libopusfile/silk ../libopusfile/silk/float ../libmad ../native_midi
CFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))
RCFILE = sdlpal.rc
RCFILE_EXT = sdlpal-preprocess.rc
OBJFILES = $(CFILES:.c=$(INTER).o) $(CPPFILES:.cpp=$(INTER).o) $(RCFILE_EXT:.rc=$(INTER).o)
DEPFILES = $(CFILES:.c=$(INTER).d) $(CPPFILES:.cpp=$(INTER).d)
TEST_CPPFILES = $(wildcard ../tests/*.cpp)
TEST_OBJFILES = $(TEST_CPPFILES:.cpp=.o)
SDL_CONFIG = sdl2-config

override CCFLAGS += `$(SDL_CONFIG) --cflags` -I../3rd/mingw-std-threads -g -msse2 -Wall -O2 -fno-strict-aliasing $(EXTRA_CCFLAGS) $(TEST_CCFLAGS) $(GENERATED)
CXXFLAGS = $(CCFLAGS) -std=c++11
CFLAGS = $(CCFLAGS) -std=gnu99
OPENGL_LIBS = -lopengl32
LDFLAGS = `$(SDL_CONFIG) --libs` ${OPENGL_LIBS} -lm -lwinmm -lole32 -loleaut32 -limm32 -lcomctl32 -luuid -ldxguid -lversion -lsetupapi -static -static-libgcc -static-libstdc++
TEST_CXXFLAGS += -isystem $(GTEST_DIR)/include -I $(GTEST_DIR) -g -Wall -Wextra -pthread

POSTCOMPILE = @mv -f $*$(INTER).Td $*$(INTER).d && touch $@

.PHONY : all clean check

all: $(TARGET)

$(TARGET): $(OBJFILES)
	@echo [LD] $@
	@$(HOST)g++ $^ -o $@ $(LDFLAGS)

gtest-all.o : $(GTEST_DIR)/src/gtest-all.cc %.d
	@echo [CC] $<
	@$(HOST)g++ $(DEPFLAGS) $(TEST_CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

${RCFILE_EXT} : ${RCFILE}
	@SLF_EXTENSION=`grep 'define PAL_LOCALIZATION_EXT' ../defines.h | awk '{print $$4}' | sed s/\"//g`
	@sed s/'" PAL_LOCALIZATION_EXT "'/${SLF_EXTENSION}/g $< > $@

%$(INTER).o: %.c %$(INTER).d
	@echo [CC] $<
	@$(HOST)gcc $(DEPFLAGS) $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%$(INTER).o: %.cpp %$(INTER).d
	@echo [CC] $<
	@$(HOST)g++ $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%$(INTER).o: %.rc
	@echo [RES] $^
	@$(HOST)windres -I.. -i $^ -o $@

$(TEST_TARGET): $(OBJFILES) $(TEST_OBJFILES) gtest-all.o
	@echo [LD] $@
	@$(HOST)g++ $^ -o $@ $(LDFLAGS) -pthread

clean:
	-rm -f $(TARGET) $(TEST_TARGET) $(OBJFILES) $(TEST_OBJFILES) $(DEPFILES) ${RCFILE_EXT}

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)

../liboggvorbis/%$(INTER).o: EXTRA_CCFLAGS := -I../liboggvorbis/include -I../liboggvorbis/src
../libopusfile/%$(INTER).o: EXTRA_CCFLAGS := -I../liboggvorbis/include -I../libopusfile/include -I../libopusfile/src -I../libopusfile/celt -I../libopusfile/silk -I../libopusfile/silk/float -DHAVE_CONFIG_H
%$(INTER).o: EXTRA_CCFLAGS := -I. -I.. -I../liboggvorbis/include -I../libopusfile/include -DPAL_HAS_PLATFORM_SPECIFIC_UTILS
check: TEST_CCFLAGS = -I. -I.. -DUNIT_TEST=1 -isystem $(GTEST_DIR)/include
check: $(TEST_TARGET)
	@echo [EXEC] $(TEST_TARGET)
	@chmod +x $(TEST_TARGET)
	@exec $(TEST_TARGET)
