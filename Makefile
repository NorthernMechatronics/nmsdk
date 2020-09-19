SDKROOT?= .
include $(SDKROOT)/makedefs/nm_common.mk

BUILDDIR := $(SDKROOT)/build

AM_HAL_DIR    := $(SDKROOT)/features/hal
AM_UTILS_DIR  := $(SDKROOT)/features/utils
FREERTOS_DIR     := $(SDKROOT)/features/FreeRTOS
FREERTOS-CLI_DIR := $(SDKROOT)/features/FreeRTOS-Plus-CLI

ifdef DEBUG
    AM_HAL_TARGET   = libam_hal-dev.a
    AM_UTILS_TARGET = libam_utils-dev.a
    FREERTOS_TARGET     = libfreertos-dev.a
    FREERTOS-CLI_TARGET = libfreertos-cli-dev.a
    CONFIG = debug
else
    AM_HAL_TARGET   = libam_hal.a
    AM_UTILS_TARGET = libam_utils.a
    FREERTOS_TARGET     = libfreertos.a
    FREERTOS-CLI_TARGET = libfreertos-cli.a
    CONFIG = release
endif

all: $(BUILDDIR) am_hal am_utils freertos freertos-cli
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

freertos: $(BUILDDIR)/$(FREERTOS_TARGET)
$(BUILDDIR)/$(FREERTOS_TARGET): $(FREERTOS_DIR)
	$(MAKE) -C $<
	$(CP) $(FREERTOS_DIR)/$(CONFIG)/$(FREERTOS_TARGET) $@

freertos-cli: $(BUILDDIR)/$(FREERTOS-CLI_TARGET)
$(BUILDDIR)/$(FREERTOS-CLI_TARGET): $(FREERTOS-CLI_DIR)
	$(MAKE) -C $<
	$(CP) $(FREERTOS-CLI_DIR)/$(CONFIG)/$(FREERTOS-CLI_TARGET) $@

clean:
	$(MAKE) -C $(AM_HAL_DIR) clean
	$(MAKE) -C $(FREERTOS_DIR) clean
	$(MAKE) -C $(FREERTOS-CLI_DIR) clean
	$(RM) -f $(BUILDDIR)/$(AM_HAL_TARGET) $(BUILDDIR)/$(AM_UTILS_TARGET) $(BUILDDIR)/$(FREERTOS_TARGET) $(BUILDDIR)/$(FREERTOS-CLI_TARGET)

.PHONY: hal freertos
.PHONY: clean
