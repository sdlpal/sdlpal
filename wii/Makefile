TARGET = boot

HOST = powerpc-eabi-

DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td

SOURCES = . .. ../adplug
CFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c)) 
CPPFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))
OFILES = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
DEPFILES = $(OFILES:.o=.d) 

CFLAGS = -g -Wall -O2 `sdl-config --cflags` -DGEKKO -DPAL_HAS_PLATFORM_SPECIFIC_UTILS -I. -I../ -I../liboggvorbis/include -I../liboggvorbis/src $(DEPFLAGS) 
CXXFLAGS = $(CFLAGS)
LDFLAGS = `sdl-config --libs` -DGEKKO -mrvl

POSTCOMPILE = @mv -f $*.Td $*.d && touch $@

$(TARGET): $(OFILES)
	@echo [LD] $@
	@$(HOST)g++ $(OFILES) -o $@.elf $(LDFLAGS)
	@elf2dol $(TARGET).elf $(TARGET).dol

%.o: %.c %.d
	@echo [CC] $<
	@$(HOST)gcc $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%.o: %.cpp %.d
	@echo [CC] $<
	@$(HOST)g++ $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

clean:
	rm -f $(TARGET).elf $(TARGET).dol $(OFILES) $(DEPFILES)

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)

