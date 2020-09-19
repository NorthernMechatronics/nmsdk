SDKROOT?=..

ifndef AMBIQ_SDK
  AMBIQ_SDK := $(SDKROOT)/../AmbiqSuite-R2.4.2
  $(warning AmbiqSuite SDK location AMBIQ_SDK not defined )
  ifneq "$(wildcard $(AMBIQ_SDK) )" ""
    $(warning found AmbiqSuite SDK at $(AMBIQ_SDK))
  else
    $(error unable to locate AmbiqSuite SDK)
  endif
endif

ifdef DEBUG
  $(info ****** DEBUG VERSION ******)
else
  $(info ****** RELEASE VERSION ******)
endif

SDKLIB   := $(SDKROOT)/lib

SHELL     := /bin/bash
TOOLCHAIN ?= arm-none-eabi
PART       = apollo3
CPU        = cortex-m4
FPU        = fpv4-sp-d16
FABI       = hard

#### Required Executables ####
CC   = $(TOOLCHAIN)-gcc
GCC  = $(TOOLCHAIN)-gcc
CPP  = $(TOOLCHAIN)-cpp
LD   = $(TOOLCHAIN)-ld
OCP  = $(TOOLCHAIN)-objcopy
OD   = $(TOOLCHAIN)-objdump
RD   = $(TOOLCHAIN)-readelf
AR   = $(TOOLCHAIN)-ar
SIZE = $(TOOLCHAIN)-size
CP   = $(shell which cp 2>/dev/null)
RM   = $(shell which rm 2>/dev/null)

DEFINES  = -Dgcc
DEFINES += -DAM_PART_APOLLO3
DEFINES += -DAM_PACKAGE_BGA
DEFINES += -DPART_apollo3
ifdef DEBUG
  DEFINES += -DAM_ASSERT_INVALID_THRESHOLD=0
  DEFINES += -DAM_DEBUG_ASSERT
endif

CFLAGS  = -mthumb -mcpu=$(CPU) -mfpu=$(FPU) -mfloat-abi=$(FABI)
CFLAGS += -ffunction-sections -fdata-sections
CFLAGS += -MMD -MP -std=c99 -Wall
CFLAGS += $(DEFINES)
ifdef DEBUG
  CFLAGS += -g -O0
else
  CFLAGS += -O3
endif
CFLAGS += 

LFLAGS  = -mthumb -mcpu=$(CPU) -mfpu=$(FPU) -mfloat-abi=$(FABI)
LFLAGS += -nostartfiles -static

OCPFLAGS = -Obinary
ODFLAGS  = -S
