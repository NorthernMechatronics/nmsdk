SDKROOT?=..
include $(SDKROOT)/makedefs/nm_common.mk

ifndef LORAMAC
  LORAMAC  := $(SDKROOT)/../loramac-node
  $(warning LoRaMAC-Node source location LORAMAC not defined )
  ifneq "$(wildcard $(LORAMAC) )" ""
    $(warning found LoRaMAC-Node at $(LORAMAC))
  else
    $(error unable to locate LoRaMAC-Node source)
  endif
endif
