SDKROOT?= .
include $(SDKROOT)/makedefs/nm_common.mk

BUILDDIR := $(SDKROOT)/build

AM_HAL_DIR    := $(SDKROOT)/features/hal
AM_UTILS_DIR  := $(SDKROOT)/features/utils

FREERTOS_DIR     := $(SDKROOT)/features/FreeRTOS
FREERTOS-CLI_DIR := $(SDKROOT)/features/FreeRTOS-Plus-CLI

CORDIO_DIR    := $(SDKROOT)/features/Cordio

LORAMAC_DIR   := $(SDKROOT)/features/loramac-node

ifdef DEBUG
    AM_HAL_TARGET   = libam_hal-dev.a
    AM_UTILS_TARGET = libam_utils-dev.a
    FREERTOS_TARGET     = libfreertos-dev.a
    FREERTOS-CLI_TARGET = libfreertos-cli-dev.a
    CORDIO_TARGET = libcordio-dev.a
    LORAMAC_TARGET = libloramac-dev.a
    CONFIG = debug
else
    AM_HAL_TARGET   = libam_hal.a
    AM_UTILS_TARGET = libam_utils.a
    FREERTOS_TARGET     = libfreertos.a
    FREERTOS-CLI_TARGET = libfreertos-cli.a
    CORDIO_TARGET = libcordio.a
    LORAMAC_TARGET = libloramac.a
    CONFIG = release
endif

export DEBUG

all: $(BUILDDIR) am_hal am_utils freertos freertos-cli cordio loramac
	@echo "****** Build Successful ******"

$(BUILDDIR):
	@mkdir -p $@

am_hal:
	$(MAKE) -C $(AM_HAL_DIR)
	$(CP) $(AM_HAL_DIR)/$(CONFIG)/$(AM_HAL_TARGET) $(BUILDDIR)/$(AM_HAL_TARGET)

am_utils:
	$(MAKE) -C $(AM_UTILS_DIR)
	$(CP) $(AM_UTILS_DIR)/$(CONFIG)/$(AM_UTILS_TARGET) $(BUILDDIR)/$(AM_UTILS_TARGET)

freertos:
	$(MAKE) -C $(FREERTOS_DIR)
	$(CP) $(FREERTOS_DIR)/$(CONFIG)/$(FREERTOS_TARGET) $(BUILDDIR)/$(FREERTOS_TARGET)

freertos-cli:
	$(MAKE) -C $(FREERTOS-CLI_DIR)
	$(CP) $(FREERTOS-CLI_DIR)/$(CONFIG)/$(FREERTOS-CLI_TARGET) $(BUILDDIR)/$(FREERTOS-CLI_TARGET)

cordio:
	$(MAKE) -C $(CORDIO_DIR)
	$(CP) $(CORDIO_DIR)/$(CONFIG)/$(CORDIO_TARGET) $(BUILDDIR)/$(CORDIO_TARGET)

loramac:
	$(MAKE) -C $(LORAMAC_DIR)
	$(CP) $(LORAMAC_DIR)/$(CONFIG)/$(LORAMAC_TARGET) $(BUILDDIR)/$(LORAMAC_TARGET)


clean:
	$(MAKE) -C $(AM_HAL_DIR) clean
	$(MAKE) -C $(FREERTOS_DIR) clean
	$(MAKE) -C $(FREERTOS-CLI_DIR) clean
	$(MAKE) -C $(CORDIO_DIR) clean
	$(MAKE) -C $(LORAMAC_DIR) clean
	$(RM) -f $(BUILDDIR)/$(AM_HAL_TARGET) $(BUILDDIR)/$(AM_UTILS_TARGET) $(BUILDDIR)/$(FREERTOS_TARGET) $(BUILDDIR)/$(FREERTOS-CLI_TARGET) $(BUILDDIR)/$(CORDIO_TARGET) $(BUILDDIR)/$(LORAMAC_TARGET)

.PHONY: hal freertos
.PHONY: clean
