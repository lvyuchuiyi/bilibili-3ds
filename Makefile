#---------------------------------------------------------------------------------
APP_TITLE       := BiliBili 3DS
APP_DESCRIPTION := BiliBili video client for 3DS
APP_AUTHOR      := Codex
.SUFFIXES:

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET     := bilibili3ds
BUILD      := build
SOURCES    := source
DATA       := data
INCLUDES   := source
ROMFS      := romfs

ARCH   := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft

CFLAGS := -g -Wall -O2 -mword-relocations \
          -fomit-frame-pointer -ffunction-sections \
          $(ARCH)

CFLAGS += $(INCLUDE) -DARM11 -D_3DS

CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++11

ASFLAGS := -g $(ARCH)
LDFLAGS  = -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS := -lcitro3d -lctru -lmbedtls -lmbedcrypto -lmbedx509 -lm -lz

LIBDIRS := $(CTRULIB) $(PORTLIBS)

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>devkitPro")
endif

export ROMFS
INCLUDES += $(ROMFS)
LIBDIRS := $(CTRULIB) $(PORTLIBS)

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

else
CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export OUTPUT := $(CURDIR)/$(TARGET)

check: $(OUTPUT).3dsx
	@3dslink $(OUTPUT).3dsx

$(OUTPUT).3dsx: $(OUTPUT).elf $(OUTPUT).smdh

#---------------------------------------------------------------------------------
# CIA build using makerom (included with devkitPro 3ds-dev package)
#---------------------------------------------------------------------------------
.PHONY: cia

cia: $(OUTPUT).cia

$(OUTPUT).cia: $(OUTPUT).elf $(OUTPUT).smdh
	@echo building CIA ...
	@makerom -f cia -o $(OUTPUT).cia -elf $(OUTPUT).elf -smdh $(OUTPUT).smdh
	@echo "  $(notdir $(OUTPUT)).cia ready"

%.bin.o: %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPSDIR)/*.d
endif
