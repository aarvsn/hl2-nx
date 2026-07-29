#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
#---------------------------------------------------------------------------------
TARGET          :=      hl2_nx
APP_TITLE       :=      Half-Life 2
APP_AUTHOR      :=      aarvsn
APP_VERSION     :=      1.1.0
BUILD           :=      build
SOURCES         :=      source
DATA            :=      data
INCLUDES        :=      include
ROMFS           :=      romfs

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH    :=      -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE

# Optimisation pipeline:
#   -O3                 : aggressive inlining / vectorisation / loop transforms
#   -ffast-math         : the Source engine already assumes IEEE-leans; safe and faster
#   -funsafe-math-optimizations : reciprocal approximations, no signaling NaN traps
#   -ffunction-sections / -fdata-sections : pair with --gc-sections below
#   -flto=auto          : link-time optimisation across all TUs (smaller + faster)
#   -fno-plt            : indirect calls via GOT directly (saves a PLT hop)
#   -fno-stack-protector: Source modules don't honour it anyway and it costs a TLS load
#                         per function; the fake-Android TLS shim has no canary to read.
OPTFLAGS := -O3 -ffast-math -funsafe-math-optimizations \
            -ffunction-sections -fdata-sections \
            -flto=auto -fno-plt -fno-stack-protector

WARNFLAGS := -Wall -Wextra -Wno-unused-parameter -Wno-unused-function \
             -Wno-missing-field-initializers -Wno-sign-compare

CFLAGS  :=      $(OPTFLAGS) $(WARNFLAGS) $(ARCH) $(DEFINES)
CFLAGS  +=      $(INCLUDE) -D__SWITCH__

CXXFLAGS :=     $(CFLAGS)

ASFLAGS :=      -g $(ARCH)

# Linker:
#   --gc-sections   : drop unreferenced functions/data (huge win on a shim this small)
#   --as-needed     : don't link a lib just because it appears on the line
#   -flto=auto      : must match compile-time -flto
#   -Wl,-O1         : cheaper than -O2, picks up --gc-sections + --as-needed
LDFLAGS :=      -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) \
                -Wl,-Map,$(notdir $*.map) \
                -Wl,--gc-sections -Wl,--as-needed -Wl,-O1 -flto=auto

# switch-sdl2 provides the SDL_* surface the Source modules import;
# switch-mesa (EGL/GLES) backs the dlopen("libEGL.so"/"libGLESv3.so")
# pseudo-modules togl resolves GL entry points from.
# switch-ffmpeg decodes the startup videos (Valve logo / Bink); its libs have
# circular deps so wrap them in a link group. bz2/dav1d are avcodec deps.
LIBS    := -lSDL2_image -ljpeg -lpng -lwebp \
                -lSDL2 -lEGL -lGLESv2 -lglapi -ldrm_nouveau \
                -Wl,--start-group -lavformat -lavcodec -lswscale -lswresample -lavutil -lbz2 -ldav1d -Wl,--end-group \
                -lz -lnx -lm

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS := $(PORTLIBS) $(LIBNX)


#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT   :=      $(CURDIR)/$(TARGET)
export TOPDIR   :=      $(CURDIR)

export VPATH    :=      $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
                        $(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR  :=      $(CURDIR)/$(BUILD)

CFILES          :=      $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES        :=      $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES          :=      $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES        :=      $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# link with CXX: mesa's libEGL.a (nouveau shader compiler) is C++
#---------------------------------------------------------------------------------
export LD       :=      $(CXX)

export OFILES_BIN       :=      $(addsuffix .o,$(BINFILES))
export OFILES_SRC       :=      $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES   :=      $(OFILES_BIN) $(OFILES_SRC)
export HFILES_BIN       :=      $(addsuffix .h,$(subst .,_,$(BINFILES)))

export INCLUDE  :=      $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
                        $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                        -I$(CURDIR)/$(BUILD)

export LIBPATHS :=      $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

ifeq ($(strip $(ICON)),)
        icons := $(wildcard *.jpg)
        ifneq (,$(findstring $(TARGET).jpg,$(icons)))
                export APP_ICON := $(TOPDIR)/$(TARGET).jpg
        else
                ifneq (,$(findstring icon.jpg,$(icons)))
                        export APP_ICON := $(TOPDIR)/icon.jpg
                endif
        endif
else
        export APP_ICON := $(TOPDIR)/$(ICON)
endif

ifeq ($(strip $(NO_ICON)),)
        export NROFLAGS += --icon=$(APP_ICON)
endif

ifeq ($(strip $(NO_NACP)),)
        export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp
endif

ifneq ($(ROMFS),)
        export NROFLAGS += --romfsdir=$(CURDIR)/$(ROMFS)
endif

ifneq ($(APP_TITLEID),)
        export NACPFLAGS += --titleid=$(APP_TITLEID)
endif

# Parallel build: devkitPro's switch_rules already uses $(MAKE) -j; just expose a knob.
NPROCS ?= $(shell nproc 2>/dev/null || echo 4)
MAKEFLAGS += -j$(NPROCS)

.PHONY: $(BUILD) clean all

#---------------------------------------------------------------------------------
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nro $(TARGET).nacp $(TARGET).elf

#---------------------------------------------------------------------------------
# Convenience targets
#---------------------------------------------------------------------------------
run: all
	@echo "Build complete: $(TARGET).nro"
	@echo "Copy /switch/hl2_nx/$(TARGET).nro to your Switch SD card."

size: all
	@ls -lh $(TARGET).nro 2>/dev/null || echo "no nro"

#---------------------------------------------------------------------------------
else
.PHONY: all

DEPENDS :=      $(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
all     :       $(OUTPUT).nro

ifeq ($(strip $(NO_NACP)),)
$(OUTPUT).nro   :       $(OUTPUT).elf $(OUTPUT).nacp
else
$(OUTPUT).nro   :       $(OUTPUT).elf
endif

$(OUTPUT).elf   :       $(OFILES)

$(OFILES_SRC)   : $(HFILES_BIN)

#---------------------------------------------------------------------------------
# you need a rule like this for each extension you use as binary data
#---------------------------------------------------------------------------------
%.bin.o %_bin.h :       %.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)

#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
