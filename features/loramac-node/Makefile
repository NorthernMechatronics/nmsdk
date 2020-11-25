SDKROOT?=../..
include $(SDKROOT)/makedefs/nm_loramac.mk

ifdef DEBUG
    TARGET := libloramac-dev
    CONFIG := ./debug
else
    TARGET := libloramac
    CONFIG := ./release
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

INCLUDES += -I$(LORAMAC)/src/apps/LoRaMac/common
INCLUDES += -I$(LORAMAC)/src/apps/LoRaMac/common/LmHandler
INCLUDES += -I$(LORAMAC)/src/apps/LoRaMac/common/LmHandler/packages

VPATH  = $(LORAMAC)/src/radio/sx126x
VPATH += $(LORAMAC)/src/boards/mcu
VPATH += $(LORAMAC)/src/mac
VPATH += $(LORAMAC)/src/mac/region
VPATH += $(LORAMAC)/src/system
VPATH += $(LORAMAC)/src/peripherals/soft-se

VPATH += $(LORAMAC)/src/apps/LoRaMac/common
VPATH += $(LORAMAC)/src/apps/LoRaMac/common/LmHandler
VPATH += $(LORAMAC)/src/apps/LoRaMac/common/LmHandler/packages


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
SRC += eeprom-board.c
#SRC += gpio-board.c
SRC += rtc-board.c
SRC += sx1262-board.c

SRC += delay.c
SRC += eeprom.c
SRC += nvmm.c
SRC += systime.c
SRC += timer.c

# LoRaMAC Application Layer Handler
SRC += NvmCtxMgmt.c
SRC += LmHandler.c
SRC += FragDecoder.c
SRC += LmhpClockSync.c
SRC += LmhpCompliance.c
SRC += LmhpFragmentation.c
SRC += LmhpRemoteMcastSetup.c


CSRC = $(filter %.c, $(SRC))
ASRC = $(filter %.s, $(SRC))

OBJS  = $(CSRC:%.c=$(CONFIG)/%.o)
OBJS += $(ASRC:%.s=$(CONFIG)/%.o)

DEPS  = $(CSRC:%.c=$(CONFIG)/%.d)
DEPS += $(ASRC:%.s=$(CONFIG)/%.d)

SPECIAL_SRC = LmHandlerMsgDisplay.c
SPECIAL_OBJ = $(CONFIG)/LmHandlerMsgDisplay.o
SPECIAL_DEP = $(CONFIG)/LmHandlerMsgDisplay.d

DEPS += $(SPECIAL_DEP)

CFLAGS += $(INCLUDES)

all: directories $(CONFIG)/$(TARGET).a

directories: $(CONFIG)

$(CONFIG):
	@$(MKDIR) $@

$(SPECIAL_OBJ): $(SPECIAL_SRC) $(SPECIAL_DEP) $(INCS)
	@echo "Compiling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) -Dprintf=am_util_stdio_printf $< -o $@

$(CONFIG)/%.o: %.c $(CONFIG)/%.d $(INCS)
	@echo "Compiling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/%.o: %.s $(CONFIG)/%.d $(INCS)
	@echo "Assembling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/$(TARGET).a: $(OBJS) $(SPECIAL_OBJ)
	$(AR) rsvc $@ $(OBJS) $(SPECIAL_OBJ)

clean:
	@echo "Cleaning..."
	$(RM) -f $(OBJS) $(DEPS) $(CONFIG)/$(TARGET).a
	$(RM) -rf $(CONFIG)

$(CONFIG)/%.d: ;

-include $(DEPS)
