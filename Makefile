#---------------------------------------------------------------------------------
APP_TITLE       := BiliBili 3DS
APP_DESCRIPTION := BiliBili video client for 3DS
APP_AUTHOR      := Codex
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

#---------------------------------------------------------------------------------
# TARGET, BUILD, SOURCES
#---------------------------------------------------------------------------------
TARGET     := bilibili3ds
BUILD      := build
SOURCES    := source
DATA       := data
INCLUDES   := source
ROMFS      := romfs

ARCH   := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -Wall -O2 -mword-relocations \
          -fomit-frame-pointer -ffunction-sections $(ARCH)

CFLAGS += $(INCLUDE) -D__3DS__

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS := -g $(ARCH)
LDFLAGS  = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS := -lcitro2d -lcitro3d -lmbedtls -lmbedx509 -lmbedcrypto -lctru -lm -lz

LIBDIRS := $(CTRULIB) $(PORTLIBS)

#---------------------------------------------------------------------------------
# The recursive make split
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)

export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif

export OFILES := $(addsuffix .o,$(BINFILES)) \
                 $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)

export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ROMFS)),)
export NITROFLAGS :=
else
export NITROFLAGS := -d $(ROMFS)
endif

.PHONY: $(SOURCES) clean all cia release

all: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

cia: $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile cia

release: cia

$(BUILD):
	@[ -d $@ ] || mkdir -p $@

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).3dsx $(TARGET).elf $(TARGET).smdh $(TARGET).cia

check: all
	@3dslink $(TARGET).3dsx

install: all
	@echo "Copy $(TARGET).3dsx to SD card root"

$(SOURCES):
	@echo "  $(TARGET).3dsx ready"

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))


#---------------------------------------------------------------------------------
# Main targets - actual compilation
#---------------------------------------------------------------------------------
$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh

$(OUTPUT).elf: $(OFILES)

#---------------------------------------------------------------------------------
# CIA build
#---------------------------------------------------------------------------------
.PHONY: cia

cia: $(OUTPUT).3dsx
	@if command -v makerom >/dev/null 2>&1; then \
		echo building CIA ...; \
		makerom -f cia -o $(OUTPUT).cia -elf $(OUTPUT).elf -smdh $(OUTPUT).smdh; \
		echo "  $(notdir $(OUTPUT)).cia ready"; \
	else \
		echo "  makerom not found, skipping CIA (3dsx is available)"; \
	fi

check: $(OUTPUT).3dsx
	@3dslink $(OUTPUT).3dsx

#---------------------------------------------------------------------------------
# Binary data rules
#---------------------------------------------------------------------------------
%.bin.o: %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPSDIR)/*.d

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------




