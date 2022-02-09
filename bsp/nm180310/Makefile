SDKROOT?=../..

include $(SDKROOT)/makedefs/nm_common.mk

ifdef DEBUG
    TARGET  := libam_bsp-dev.a
    CONFIG  := ./debug
else
    TARGET  := libam_bsp.a
    CONFIG  := ./release
endif
COMPILERNAME := gcc
PROJECT      := libam_bsp_gcc

TARGET_C := am_bsp_pins.c
TARGET_H := am_bsp_pins.h

BSP_GENERATOR = $(AMBIQ_SDK)/tools/bsp_generator/pinconfig.py

INCLUDES  = -I$(AMBIQ_SDK)/CMSIS/AmbiqMicro/Include
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/ARM/Include
INCLUDES += -I$(AMBIQ_SDK)/mcu/apollo3
INCLUDES += -I$(AMBIQ_SDK)/mcu/apollo3/hal
INCLUDES += -I$(AMBIQ_SDK)/mcu/apollo3/regs
INCLUDES += -I$(AMBIQ_SDK)/utils
INCLUDES += -I$(SDKROOT)/bsp/devices
INCLUDES += -I.

VPATH  = .
VPATH += $(SDKROOT)/bsp/devices

SRC  = am_bsp.c
SRC += am_bsp_pins.c
SRC += am_devices_button.c
SRC += am_devices_led.c
SRC += nm_devices_sx1262.c

CSRC = $(filter %.c, $(SRC))
ASRC = $(filter %.s, $(SRC))

OBJS = $(CSRC:%.c=$(CONFIG)/%.o)
OBJS+= $(ASRC:%.s=$(CONFIG)/%.o)

DEPS = $(CSRC:%.c=$(CONFIG)/%.d)
DEPS+= $(ASRC:%.s=$(CONFIG)/%.d)

CFLAGS += $(INCLUDES)
CFLAGS += $(DEFINES)

all: directories $(CONFIG)/$(TARGET)

directories: $(CONFIG)

$(CONFIG): $(TARGET_H) $(TARGET_C)
	@$(MKDIR) $@

$(TARGET_H): bsp_pins.src
	python $(BSP_GENERATOR) $< h > $@

$(TARGET_C): bsp_pins.src
	python $(BSP_GENERATOR) $< c > $@

$(CONFIG)/%.o: %.c $(CONFIG)/%.d
	@echo "Compiling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/%.o: %.s $(CONFIG)/%.d
	@echo "Assembling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/$(TARGET): $(OBJS)
	$(AR) rsvc $@ $(OBJS)

clean:
	$(RM) -f $(TARGET_C) $(TARGET_H) $(CONFIG)/$(TARGET)
	$(RM) -rf $(CONFIG)

$(CONFIG)/%.d: ;

-include $(DEPS)
