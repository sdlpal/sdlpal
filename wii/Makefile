TARGET = boot.dol

INTER=.wii

HOST = powerpc-eabi-

DEPFLAGS = -MT $@ -MMD -MP -MF $*$(INTER).Td

SOURCES = . .. ../adplug ../liboggvorbis/src ../libmad
CFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c)) 
CPPFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))
OFILES = $(CFILES:.c=$(INTER).o) $(CPPFILES:.cpp=$(INTER).o)
DEPFILES = $(OFILES:.o=.d) 

CCFLAGS = $(DFALG) -g -Wall -O2 -DGEKKO -D__POWERPC__ -D__ppc__ -DPAL_HAS_PLATFORM_SPECIFIC_UTILS -I. -I../ -I../liboggvorbis/include -I../liboggvorbis/src $(DEPFLAGS) -I$(DEVKITPRO)/libogc/include -I$(DEVKITPRO)/libogc/include/ogc -I$(DEVKITPRO)/libogc/include/SDL 
CXXFLAGS = $(CCFLAGS) -std=c++11
CFLAGS = $(CCFLAGS) -std=gnu99
LDFLAGS = -L$(DEVKITPRO)/libogc/lib/wii -lSDL -lfat -lwiiuse -lbte -logc -lm -lwiikeyboard -lg -mrvl

ifeq ($(DEBUG),1)
	CCFLAGS += -DDEBUG -I$(DEVKITPRO)/wii_dev_debug/net_print/sockettest/source
	LDFLAGS += -ldb
	OFILES += $(DEVKITPRO)/wii_dev_debug/net_print/sockettest/build/net_print.o
endif

POSTCOMPILE = @mv -f $*$(INTER).Td $*$(INTER).d && touch $@

.PHONY : all clean check

all: $(TARGET)

debug:
	$(MAKE) debug-clean
	$(MAKE) "DEBUG=1" -C .

debug-clean:
	-@rm pal_utils.wii.*

test: $(TARGET).elf
	wiiload $<

$(TARGET).elf: $(OFILES)
	@echo [LD] $@
	@$(HOST)g++ $(OFILES) -o $@ $(LDFLAGS)

$(TARGET): $(TARGET).elf
	@echo [elf2dol] $@
	@elf2dol $< $(TARGET)

%$(INTER).o: %.c %$(INTER).d
	@echo [CC] $<
	@$(HOST)gcc $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%$(INTER).o: %.cpp %$(INTER).d
	@echo [CC] $<
	@$(HOST)g++ $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

clean:
	-rm -f $(TARGET).elf $(TARGET) $(OFILES) $(DEPFILES)

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)

