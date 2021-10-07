SDKROOT?=../..
include $(SDKROOT)/makedefs/nm_freertos.mk

ifdef DEBUG
    TARGET := libfreertos-dev
    CONFIG := ./debug
else
    TARGET := libfreertos
    CONFIG := ./release
endif
COMPILERNAME := gcc
PROJECT      := libfreertos_gcc

INCLUDES  = -I$(AMBIQ_SDK)/mcu/apollo3
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/AmbiqMicro/Include
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/ARM/Include
INCLUDES += -I$(FREERTOS)/include

VPATH  = $(FREERTOS)
VPATH += $(FREERTOS)/portable/Common
VPATH += $(FREERTOS)/portable/MemMang

INCLUDES += -I.
VPATH    += .

SRC  = croutine.c
SRC += event_groups.c
SRC += list.c
SRC += queue.c
SRC += stream_buffer.c
SRC += tasks.c
SRC += timers.c
SRC += mpu_wrappers.c
SRC += heap_4.c
SRC += port.c

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
