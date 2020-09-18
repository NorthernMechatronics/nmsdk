SDKROOT?=../..

include $(SDKROOT)/makedefs/nm_common.mk

TARGET_C := am_bsp_pins.c
TARGET_H := am_bsp_pins.h

BSP_GENERATOR = $(AMBIQ_SDK)/tools/bsp_generator/pinconfig.py

all: $(TARGET_C) $(TARGET_H)

$(TARGET_H): bsp_pins.src
	python $(BSP_GENERATOR) $< h > $@

$(TARGET_C): bsp_pins.src
	python $(BSP_GENERATOR) $< c > $@

clean:
	$(RM) -f $(TARGET_C) $(TARGET_H) 
