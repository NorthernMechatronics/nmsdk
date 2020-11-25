SDKROOT?=../..
include $(SDKROOT)/makedefs/nm_common.mk

ifdef DEBUG
    TARGET   := libam_hal-dev
    CONFIG   := ./debug
else
    TARGET   := libam_hal
    CONFIG   := ./release
    DEFINES  += -DAM_HAL_DISABLE_API_VALIDATION
endif

COMPILERNAME := gcc
PROJECT      := libam_hal_gcc

INCLUDES = -I$(AMBIQ_SDK)/CMSIS/AmbiqMicro/Include
INCLUDES+= -I$(AMBIQ_SDK)/CMSIS/ARM/Include
INCLUDES+= -I$(AMBIQ_SDK)/mcu/apollo3
INCLUDES+= -I$(AMBIQ_SDK)/mcu/apollo3/hal
INCLUDES+= -I$(AMBIQ_SDK)/mcu/apollo3/regs

VPATH  = $(AMBIQ_SDK)/mcu/apollo3/hal
VPATH += $(AMBIQ_SDK)/mcu/apollo3/regs

SRC  = am_hal_adc.c
SRC += am_hal_ble.c
SRC += am_hal_ble_patch.c
SRC += am_hal_ble_patch_b0.c
SRC += am_hal_burst.c
SRC += am_hal_cachectrl.c
SRC += am_hal_clkgen.c
SRC += am_hal_cmdq.c
SRC += am_hal_ctimer.c
SRC += am_hal_debug.c
SRC += am_hal_flash.c
SRC += am_hal_global.c
SRC += am_hal_gpio.c
SRC += am_hal_interrupt.c
SRC += am_hal_iom.c
SRC += am_hal_ios.c
SRC += am_hal_itm.c
SRC += am_hal_mcuctrl.c
SRC += am_hal_mspi.c
SRC += am_hal_pdm.c
SRC += am_hal_pwrctrl.c
SRC += am_hal_queue.c
SRC += am_hal_reset.c
SRC += am_hal_rtc.c
SRC += am_hal_scard.c
SRC += am_hal_secure_ota.c
SRC += am_hal_security.c
SRC += am_hal_stimer.c
SRC += am_hal_sysctrl.c
SRC += am_hal_systick.c
SRC += am_hal_tpiu.c
SRC += am_hal_uart.c
SRC += am_hal_wdt.c

CSRC = $(filter %.c,$(SRC))
ASRC = $(filter %.s,$(SRC))

OBJS = $(CSRC:%.c=$(CONFIG)/%.o)
OBJS+= $(ASRC:%.s=$(CONFIG)/%.o)

DEPS = $(CSRC:%.c=$(CONFIG)/%.d)
DEPS+= $(ASRC:%.s=$(CONFIG)/%.d)

CFLAGS += $(INCLUDES)
CFLAGS += $(DEFINES)

all: directories $(CONFIG)/$(TARGET).a

directories: $(CONFIG)

$(CONFIG):
	@$(MKDIR) $@

$(CONFIG)/%.o: %.c $(CONFIG)/%.d $(INCS)
	@echo " Compiling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/%.o: %.s $(CONFIG)/%.d $(INCS)
	@echo " Assembling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/$(TARGET).a: $(OBJS)
	@echo " Library $(COMPILERNAME) $@"
	$(AR) rsvc $@ $(OBJS)

clean:
	@echo "Cleaning..."
	$(RM) -f $(OBJS) $(DEPS) $(CONFIG)/$(TARGET).a
	$(RM) -rf $(CONFIG)

$(CONFIG)/%.d: ;

# Automatically include any generated dependencies
-include $(DEPS)
.PHONY: all clean directories
