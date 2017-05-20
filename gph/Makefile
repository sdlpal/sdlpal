BUILD_GP2XWIZ = NO
BUILD_CAANOO = YES

TARGET = sdlpal.gpe

ifeq ($(BUILD_GP2XWIZ), YES)
HOST = arm-open2x-linux-
DEVICE = -DGP2XWIZ
else
ifeq ($(BUILD_CAANOO), YES)
HOST = arm-gph-linux-gnueabi-
DEVICE = -DCAANOO
endif
endif

DEPFLAGS = -MT $@ -MMD -MP -MF $*.Td

SOURCES = . .. ../adplug
CFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES = $(foreach dir, $(SOURCES), $(wildcard $(dir)/*.cpp))
OFILES = $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
DEPFILES = $(OFILES:.o=.d) 

CFLAGS = -g -Wall -O2 `sdl-config --cflags` -DGPH $(DEVICE) -DPAL_HAS_PLATFORM_SPECIFIC_UTILS -I. -I../ -I../liboggvorbis/include -I../liboggvorbis/src $(DEPFLAGS)
CXXFLAGS = $(CFLAGS)
LDFLAGS = `sdl-config --libs`

POSTCOMPILE = @mv -f $*.Td $*.d && touch $@

$(TARGET): $(OFILES)
	@echo [LD] $@
	@$(HOST)g++ $(OFILES) -o $@ $(LDFLAGS)

%.o: %.c
	@echo [CC] $<
	@$(HOST)gcc $(CFLAGS) -c $< -o $@
	$(POSTCOMPILE)

%.o: %.cpp
	@echo [CC] $<
	@$(HOST)g++ $(CXXFLAGS) -c $< -o $@
	$(POSTCOMPILE)

clean:
	rm -f $(TARGET) $(OFILES)

%.d: ;
.PRECIOUS: %.d

-include $(DEPFILES)
