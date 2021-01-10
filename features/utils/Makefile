SDKROOT?=../..
include $(SDKROOT)/makedefs/nm_common.mk

ifdef DEBUG
    TARGET       := libam_utils-dev
    CONFIG       := ./debug
    DEFINES     += -DAM_DEBUG_PRINTF
else
    TARGET       := libam_utils
    CONFIG       := ./release
endif
COMPILERNAME := gcc
PROJECT      := libam_utils_gcc

INCLUDES  = -I$(AMBIQ_SDK)/utils
INCLUDES += -I$(AMBIQ_SDK)/mcu/apollo3
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/AmbiqMicro/Include
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/ARM/Include

VPATH  = $(AMBIQ_SDK)/utils
VPATH += .

DEFINES += -DAM_PRINTF_BUFSIZE=512

SRC  = am_util_ble.c
SRC += am_util_debug.c
SRC += am_util_delay.c
SRC += am_util_faultisr.c
SRC += am_util_id.c
# FIXME Currently broken due to incorrect DEFINES by Ambiq
# SRC += am_util_regdump.c
SRC += am_util_stdio.c
SRC += am_util_string.c
SRC += am_util_time.c

CSRC = $(filter %.c, $(SRC))
ASRC = $(filter %.s, $(SRC))

OBJS  = $(CSRC:%.c=$(CONFIG)/%.o)
OBJS += $(ASRC:%.s=$(CONFIG)/%.o)

DEPS  = $(CSRC:%.c=$(CONFIG)/%.d)
DEPS += $(ASRC:%.s=$(CONFIG)/%.d)

CFLAGS += $(INCLUDES)
CFLAGS += $(DEFINES)

all: directories $(CONFIG)/$(TARGET).a

directories: $(CONFIG)

$(CONFIG):
	@$(MKDIR) $@

$(CONFIG)/%.o: %.c $(CONFIG)/%.d $(INCS)
	@echo "Compiling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/%.o: %.s $(CONFIG)/%.d $(INCS)
	@echo "Assembling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/$(TARGET).a: $(OBJS)
	$(AR) rsvc $@ $(OBJS)

clean:
	@echo "Cleaning..."
	$(RM) -f $(OBJS) $(DEPS) $(CONFIG)/$(TARGET).a
	$(RM) -rf $(CONFIG)

$(CONFIG)/%.d: ;

-include $(DEPS)
