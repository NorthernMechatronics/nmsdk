SDKROOT?=..
include $(SDKROOT)/makedefs/nm_common.mk

ifndef UECC
  UECC  := $(AMBIQ_SDK)/third_party/uecc
  $(warning micro-ecc source location UECC not defined )
  ifneq "$(wildcard $(UECC) )" ""
    $(warning found micro-ecc at $(UECC))
  else
    $(error unable to locate micro-ecc source)
  endif
endif
