SDKROOT?=../..
include $(SDKROOT)/makedefs/nm_freertos.mk

ifdef DEBUG
    TARGET := libfreertos-cli-dev
    CONFIG := ./debug
else
    TARGET := libfreertos-cli
    CONFIG := ./release
endif
COMPILERNAME := gcc
PROJECT      := libfreertos-cli_gcc

INCLUDES  = -I$(FREERTOS)/include
INCLUDES += -I$(SDKROOT)/features/FreeRTOS-Plus-CLI
INCLUDES += -I$(SDKROOT)/features/FreeRTOS

VPATH  = $(SDKROOT)/features/FreeRTOS-Plus-CLI
VPATH += .

SRC  = FreeRTOS_CLI.c

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
