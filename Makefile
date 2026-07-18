# Native Nintendo 3DS build plus host-side deterministic tests.

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM to the devkitARM directory")
endif
ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO to the devkitPro directory")
endif

.SUFFIXES:

TOPDIR ?= $(CURDIR)
include $(DEVKITARM)/3ds_rules

TARGET      := elden-ring-3ds-demake
BUILD       := build
SOURCES     := source
DATA        := data
INCLUDES    := include
ROMFS       := romfs
APP_TITLE   := Ashen Rift
APP_DESCRIPTION := Original 3DS action-RPG homebrew demo
APP_AUTHOR  := oh-ashen-one

ARCH        := -march=armv6k -mtune=mpcore -mfloat-abi=hard -mtp=soft
CFLAGS      := -g -Wall -Wextra -Werror -O2 -mword-relocations -ffunction-sections -fdata-sections $(ARCH)
CFLAGS      += $(INCLUDE) -D__3DS__
CXXFLAGS    := $(CFLAGS) -fno-rtti -fno-exceptions -std=gnu++17
ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=3dsx.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map),--gc-sections
LIBS        := -lcitro2d -lcitro3d -lctru -lm
LIBDIRS     := $(CTRULIB)

PYTHON      ?= python3
HOST_CXX    ?= c++
HOST_BUILD  := build-host

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH  := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                 $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES       := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES     := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES       := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
PICAFILES    := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.v.pica)))
BINFILES     := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD := $(CXX)
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES_BIN := $(addsuffix .o,$(BINFILES)) $(PICAFILES:.v.pica=.shbin.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)
export HFILES := $(PICAFILES:.v.pica=_shbin.h) $(addsuffix .h,$(subst .,_,$(BINFILES)))
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export _3DSXDEPS := $(OUTPUT).smdh
export _3DSXFLAGS += --smdh=$(OUTPUT).smdh --romfs=$(CURDIR)/$(ROMFS)

.PHONY: all assets validate-assets test-host run clean

all: assets $(BUILD)
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

assets:
	@$(PYTHON) tools/generate_original_assets.py

validate-assets: assets
	@$(PYTHON) tools/validate_assets.py

test-host: $(HOST_BUILD)/core_tests
	@$(HOST_BUILD)/core_tests

$(HOST_BUILD)/core_tests: source/core.cpp tests/core_tests.cpp include/demake/core.hpp
	@mkdir -p $(HOST_BUILD)
	$(HOST_CXX) -std=c++17 -Wall -Wextra -Werror -pedantic -Iinclude source/core.cpp tests/core_tests.cpp -o $@

run: all
	@test -n "$(IP)" || (echo "Usage: make run IP=<3DS-IP>" && exit 2)
	3dslink $(TARGET).3dsx -a $(IP)

$(BUILD):
	@mkdir -p $@

clean:
	@echo clean ...
	@rm -rf $(BUILD) $(HOST_BUILD) $(TARGET).3dsx $(TARGET).smdh $(TARGET).elf $(TARGET).lst $(TARGET).map romfs/audio/ambient.pcm

else

DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).3dsx: $(OUTPUT).elf $(_3DSXDEPS)
$(OFILES_SOURCES): $(HFILES)
$(OUTPUT).elf: $(OFILES)

%.bin.o %.bin.h: %.bin
	@$(bin2o)

.PRECIOUS: %.shbin
%.shbin.o %_shbin.h: %.shbin
	$(SILENTMSG) $(notdir $<)
	@$(bin2o)

-include $(DEPSDIR)/*.d

endif
