# General makefile for generic unix & linux platforms

EE_BIN = sdlpal.elf

SOURCES = \
    .. \
    ../sdl_compat \
    ../adplug \
    ../libmad \
    ../timidity \
    ../native_midi \
    ./

CFILES   = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))

EE_OBJS = $(CFILES:.c=$(INTER).o) $(CPPFILES:.cpp=$(INTER).o)

EE_CFLAGS = -D__PS2__ -DPS2 -DHAVE_MADD_ASM -DUSE_RWOPS -DHAVE_MIXER -O2 -G0 -Wall -g -std=gnu99 -D_GNU_SOURCE

EE_INCS = -I$(PS2SDK)/ee/include -I$(PS2SDK)/ports/include/SDL2 -I$(PS2SDK)/ports/include -I$(PS2SDK)/ports/include/opus -I../sdl_compat -I../timidity -I../ -I./

EE_LDFLAGS = -L$(PS2SDK)/ports/lib -L$(PS2SDK)/iop/lib/ -L$(PS2SDK)/ee/lib/ -L$(PS2DEV)/gsKit/lib -s

EE_CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti -std=c++11 -I. -I../
EE_ASFLAGS = $(CFLAGS)

EE_LIBS = -lSDL2main -lSDL2 -lSDL2_mixer -lmodplug -lxmp -logg -lvorbis -lvorbisfile -lopus -lopusfile -laudsrv \
-lgskit -ldma -ldmakit -ldraw -ldebug -lps2_drivers -lpatches -lconfig -latomic -lstdc++ -lm -lc

all: $(EE_BIN)

clean:
	rm -f $(EE_OBJS) $(EE_BIN)

include $(PS2SDK)/samples/Makefile.pref
include $(PS2SDK)/samples/Makefile.eeglobal
