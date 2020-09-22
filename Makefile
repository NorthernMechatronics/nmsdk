SDKROOT?= .
include $(SDKROOT)/makedefs/nm_common.mk

BUILDDIR := $(SDKROOT)/build

AM_HAL_DIR    := $(SDKROOT)/features/hal
AM_UTILS_DIR  := $(SDKROOT)/features/utils
AM_BSP_DIR    := $(SDKROOT)/bsp/nm180100evb

FREERTOS_DIR     := $(SDKROOT)/features/FreeRTOS
FREERTOS-CLI_DIR := $(SDKROOT)/features/FreeRTOS-Plus-CLI

CORDIO_DIR    := $(SDKROOT)/features/Cordio

LORAMAC_DIR   := $(SDKROOT)/features/loramac-node

ifdef DEBUG
    AM_HAL_TARGET   = libam_hal-dev.a
    AM_UTILS_TARGET = libam_utils-dev.a
    AM_BSP_TARGET      = libam_bsp-dev.a
    FREERTOS_TARGET     = libfreertos-dev.a
    FREERTOS-CLI_TARGET = libfreertos-cli-dev.a
    CORDIO_TARGET = libcordio-dev.a
    LORAMAC_TARGET = libloramac-dev.a
    CONFIG = debug
else
    AM_HAL_TARGET   = libam_hal.a
    AM_UTILS_TARGET = libam_utils.a
    AM_BSP_TARGET      = libam_bsp.a
    FREERTOS_TARGET     = libfreertos.a
    FREERTOS-CLI_TARGET = libfreertos-cli.a
    CORDIO_TARGET = libcordio.a
    LORAMAC_TARGET = libloramac.a
    CONFIG = release
endif

export DEBUG

all: $(BUILDDIR) am_hal am_utils am_bsp freertos freertos-cli cordio loramac
	@echo "****** Build Successful ******"

$(BUILDDIR):
	@mkdir -p $@

am_hal: $(BUILDDIR)/$(AM_HAL_TARGET)
$(BUILDDIR)/$(AM_HAL_TARGET): $(AM_HAL_DIR)
	$(MAKE) -C $<
	$(CP) $(AM_HAL_DIR)/$(CONFIG)/$(AM_HAL_TARGET) $@

am_utils: $(BUILDDIR)/$(AM_UTILS_TARGET)
$(BUILDDIR)/$(AM_UTILS_TARGET): $(AM_UTILS_DIR)
	$(MAKE) -C $<
	$(CP) $(AM_UTILS_DIR)/$(CONFIG)/$(AM_UTILS_TARGET) $@

am_bsp: $(BUILDDIR)/$(AM_BSP_TARGET)
$(BUILDDIR)/$(AM_BSP_TARGET): $(AM_BSP_DIR)
	$(MAKE) -C $<
	$(CP) $(AM_BSP_DIR)/$(CONFIG)/$(AM_BSP_TARGET) $@

freertos: $(BUILDDIR)/$(FREERTOS_TARGET)
$(BUILDDIR)/$(FREERTOS_TARGET): $(FREERTOS_DIR)
	$(MAKE) -C $<
	$(CP) $(FREERTOS_DIR)/$(CONFIG)/$(FREERTOS_TARGET) $@

freertos-cli: $(BUILDDIR)/$(FREERTOS-CLI_TARGET)
$(BUILDDIR)/$(FREERTOS-CLI_TARGET): $(FREERTOS-CLI_DIR)
	$(MAKE) -C $<
	$(CP) $(FREERTOS-CLI_DIR)/$(CONFIG)/$(FREERTOS-CLI_TARGET) $@

cordio: $(BUILDDIR)/$(CORDIO_TARGET)
$(BUILDDIR)/$(CORDIO_TARGET): $(CORDIO_DIR)
	$(MAKE) -C $<
	$(CP) $(CORDIO_DIR)/$(CONFIG)/$(CORDIO_TARGET) $@

loramac: $(BUILDDIR)/$(LORAMAC_TARGET)
$(BUILDDIR)/$(LORAMAC_TARGET): $(LORAMAC_DIR)
	$(MAKE) -C $<
	$(CP) $(LORAMAC_DIR)/$(CONFIG)/$(LORAMAC_TARGET) $@


clean:
	$(MAKE) -C $(AM_HAL_DIR) clean
	$(MAKE) -C $(AM_BSP_DIR) clean
	$(MAKE) -C $(FREERTOS_DIR) clean
	$(MAKE) -C $(FREERTOS-CLI_DIR) clean
	$(MAKE) -C $(CORDIO_DIR) clean
	$(MAKE) -C $(LORAMAC_DIR) clean
	$(RM) -f $(BUILDDIR)/$(AM_HAL_TARGET) $(BUILDDIR)/$(AM_UTILS_TARGET) $(BUILDDIR)/$(AM_BSP_TARGET) $(BUILDDIR)/$(FREERTOS_TARGET) $(BUILDDIR)/$(FREERTOS-CLI_TARGET) $(BUILDDIR)/$(CORDIO_TARGET) $(BUILDDIR)/$(LORAMAC_TARGET)

.PHONY: hal freertos
.PHONY: clean
