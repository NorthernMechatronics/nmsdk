SDKROOT?=..

ifdef DEBUG
  $(info ****** DEBUG VERSION ******)
else
  $(info ****** RELEASE VERSION ******)
endif

SDKLIB   := $(SDKROOT)/lib

TOOLCHAIN ?= arm-none-eabi
PART       = apollo3
CPU        = cortex-m4
FPU        = fpv4-sp-d16
FABI       = hard

#### Required Executables ####
SHELL     := /bin/bash
CC   = $(TOOLCHAIN)-gcc
GCC  = $(TOOLCHAIN)-gcc
CPP  = $(TOOLCHAIN)-cpp
LD   = $(TOOLCHAIN)-ld
OCP  = $(TOOLCHAIN)-objcopy
OD   = $(TOOLCHAIN)-objdump
RD   = $(TOOLCHAIN)-readelf
AR   = $(TOOLCHAIN)-ar
SIZE = $(TOOLCHAIN)-size
SED  = sed

ifeq ($(OS), Windows_NT)
  $(info Windows Platform Detected)
  MKDIR = mkdir
  CP    = cp
  RM    = rm
else
  MKDIR = mkdir -p
  CP   = $(shell which cp 2>/dev/null)
  RM   = $(shell which rm 2>/dev/null)
endif

DEFINES  = -Dgcc
DEFINES += -DAM_PART_APOLLO3
DEFINES += -DAM_PACKAGE_BGA
DEFINES += -DPART_apollo3
ifdef DEBUG
  DEFINES += -DAM_ASSERT_INVALID_THRESHOLD=0
  DEFINES += -DAM_DEBUG_ASSERT
  DEFINES += -DAM_DEBUG_PRINTF
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
