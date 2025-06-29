# Makefile for DOS platforms with DJGPP under linux

MAKEFLAGS += -j

TARGET = sdlpal.exe
TEST_TARGET = sdlpal-tests.exe

INTER=.dos

HOST = i586-pc-msdosdjgpp-
TEST_CCFLAGS =

GENERATED = -DPAL_HAS_GIT_REVISION $(shell ../scripts/gengitrev)

DEPFLAGS = -MT $@ -MMD -MP -MF $*$(INTER).Td

GTEST_DIR = ../3rd/googletest/googletest

SOURCES = . .. ../sdl_compat ../adplug ../liboggvorbis/src ../libopusfile/src ../libopusfile/celt ../libopusfile/silk ../libopusfile/silk/float ../libmad ../native_midi ../timidity
CFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))
OBJFILES = $(CFILES:.c=$(INTER).o) $(CPPFILES:.cpp=$(INTER).o)
DEPFILES = $(CFILES:.c=$(INTER).d) $(CPPFILES:.cpp=$(INTER).d)
TEST_CPPFILES = $(wildcard ../tests/*.cpp)
TEST_OBJFILES = $(TEST_CPPFILES:.cpp=.o)
SDL_CONFIG = sdl2-config

override CCFLAGS += `$(SDL_CONFIG) --cflags` -I../sdl_compat -I../3rd/mingw-std-threads -ggdb -ansi -Wall -Ofast -fno-strict-aliasing $(EXTRA_CCFLAGS) $(TEST_CCFLAGS) $(GENERATED)
CXXFLAGS = $(CCFLAGS) -std=c++11
CFLAGS = $(CCFLAGS) -std=gnu99
OPENGL_LIBS =
LDFLAGS = `$(SDL_CONFIG) --libs` ${OPENGL_LIBS}
TEST_CXXFLAGS += -isystem $(GTEST_DIR)/include -I $(GTEST_DIR) -g -Wall -Wextra -pthread

POSTCOMPILE = @mv -f $*$(INTER).Td $*$(INTER).d && touch $@

.PHONY : all clean check

all: $(TARGET)

install: all
	md5sum $(TARGET) 
	@cp -f $(TARGET) /mnt/j/pal

$(TARGET): $(OBJFILES)
	@echo [LD] $@
	@$(HOST)g++ $^ -o $@ $(LDFLAGS)

gtest-all.o : $(GTEST_DIR)/src/gtest-all.cc %.d
	@echo [CC] $<
	@$(HOST)g++ $(DEPFLAGS) $(TEST_CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%$(INTER).o: %.c %$(INTER).d
	@echo [CC] $<
	@$(HOST)gcc $(DEPFLAGS) $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%$(INTER).o: %.cpp %$(INTER).d
	@echo [CC] $<
	@$(HOST)g++ $(DEPFLAGS) $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

$(TEST_TARGET): $(OBJFILES) $(TEST_OBJFILES) gtest-all.o
	@echo [LD] $@
	@$(HOST)g++ $^ -o $@ $(LDFLAGS) -pthread

clean:
	-rm -f $(TARGET) $(TEST_TARGET) $(OBJFILES) $(TEST_OBJFILES) $(DEPFILES)

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)

../liboggvorbis/%$(INTER).o: EXTRA_CCFLAGS := -I../liboggvorbis/include -I../liboggvorbis/src
../libopusfile/%$(INTER).o: EXTRA_CCFLAGS := -I../liboggvorbis/include -I../libopusfile/include -I../libopusfile/src -I../libopusfile/celt -I../libopusfile/silk -I../libopusfile/silk/float -DHAVE_CONFIG_H -DFLOAT_APPROX
%$(INTER).o: EXTRA_CCFLAGS := -I. -I.. -I../liboggvorbis/include -I../libopusfile/include -I../timidity -DPAL_HAS_PLATFORM_SPECIFIC_UTILS
check: TEST_CCFLAGS = -I. -I.. -DUNIT_TEST=1 -isystem $(GTEST_DIR)/include
check: $(TEST_TARGET)
	@echo [EXEC] $(TEST_TARGET)
	@chmod +x $(TEST_TARGET)
	@exec $(TEST_TARGET)
