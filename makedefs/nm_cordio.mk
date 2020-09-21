SDKROOT?=..
include $(SDKROOT)/makedefs/nm_common.mk

ifndef CORDIO
  CORDIO  := $(AMBIQ_SDK)/third_party/exactle
  $(warning Cordio source location CORDIO not defined )
  ifneq "$(wildcard $(CORDIO) )" ""
    $(warning found Cordio at $(CORDIO))
  else
    $(error unable to locate Cordio source)
  endif
endif

CORDIO_STACK    = $(CORDIO)/ble-host
CORDIO_PROFILES = $(CORDIO)/ble-profiles
CORDIO_WSF      = $(CORDIO)/wsf

DEFINES += -DSEC_ECC_CFG=SEC_ECC_CFG_UECC
