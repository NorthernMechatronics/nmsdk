SDKROOT?=../..
include $(SDKROOT)/makedefs/nm_loramac.mk

ifdef DEBUG
    TARGET       := libloramac-dev
    CONFIG       := debug
else
    TARGET       := libloramac
    CONFIG       := release
endif
COMPILERNAME := gcc
PROJECT      := libloramac_gcc

INCLUDES  = -I$(AMBIQ_SDK)/mcu/apollo3
INCLUDES += -I$(AMBIQ_SDK)/mcu/apollo3/hal
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/AmbiqMicro/Include
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/ARM/Include
INCLUDES += -I$(AMBIQ_SDK)/utils

INCLUDES += -I$(SDKROOT)/bsp/nm180100evb
INCLUDES += -I$(SDKROOT)/bsp/devices
INCLUDES += -I$(SDKROOT)/features/FreeRTOS

INCLUDES += -I$(LORAMAC)/src/radio
INCLUDES += -I$(LORAMAC)/src/radio/sx126x
INCLUDES += -I$(LORAMAC)/src/boards
INCLUDES += -I$(LORAMAC)/src/mac
INCLUDES += -I$(LORAMAC)/src/mac/region
INCLUDES += -I$(LORAMAC)/src/system
INCLUDES += -I$(LORAMAC)/src/peripherals/soft-se

VPATH  = $(LORAMAC)/src/radio/sx126x
VPATH += $(LORAMAC)/src/boards/mcu
VPATH += $(LORAMAC)/src/mac
VPATH += $(LORAMAC)/src/mac/region
VPATH += $(LORAMAC)/src/system
VPATH += $(LORAMAC)/src/peripherals/soft-se

INCLUDES += -I./src/boards/nm180100
INCLUDES += -I.

VPATH += ./src/boards/nm180100
VPATH += .

SRC  = radio.c
SRC += sx126x.c
SRC += utilities.c

SRC += LoRaMacAdr.c
SRC += LoRaMac.c
SRC += LoRaMacClassB.c
SRC += LoRaMacCommands.c
SRC += LoRaMacConfirmQueue.c
SRC += LoRaMacCrypto.c
SRC += LoRaMacParser.c
SRC += LoRaMacSerializer.c

SRC += RegionAS923.c
SRC += RegionAU915.c
SRC += RegionBaseUS.c
SRC += Region.c
SRC += RegionCN470.c
SRC += RegionCN779.c
SRC += RegionCommon.c
SRC += RegionEU433.c
SRC += RegionEU868.c
SRC += RegionIN865.c
SRC += RegionKR920.c
SRC += RegionRU864.c
SRC += RegionUS915.c

SRC += board.c
SRC += delay-board.c
#SRC += gpio-board.c
SRC += rtc-board.c
SRC += sx1262-board.c

#SRC += aes.c
#SRC += cmac.c
#SRC += soft-se.c
#SRC += soft-se-hal.c

SRC += delay.c
SRC += systime.c
SRC += timer.c

CSRC = $(filter %.c, $(SRC))
ASRC = $(filter %.s, $(SRC))

OBJS  = $(CSRC:%.c=$(CONFIG)/%.o)
OBJS += $(ASRC:%.s=$(CONFIG)/%.o)

DEPS  = $(CSRC:%.c=$(CONFIG)/%.d)
DEPS += $(ASRC:%.s=$(CONFIG)/%.d)

CFLAGS += $(INCLUDES)

all: directories $(CONFIG)/$(TARGET).a

directories: $(CONFIG)

$(CONFIG):
	@mkdir -p $@

$(CONFIG)/%.o: %.c $(CONFIG)/%.d $(INCS)
	@echo "Compiling $(COMPILERNAME) $<" ;\
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/%.o: %.s $(CONFIG)/%.d $(INCS)
	@echo "Assembling $(COMPILERNAME) $<" ;\
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/$(TARGET).a: $(OBJS)
	$(AR) rsvc $@ $(OBJS)

clean:
	@echo "Cleaning..." ;\
	$(RM) -f $(OBJS) $(DEPS) $(CONFIG)/$(TARGET).a ;\
	$(RM) -rf $(CONFIG)

$(CONFIG)/%.d: ;

-include $(DEPS)
