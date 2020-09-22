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

DEFINES += -D"REGION_AS923"
DEFINES += -D"REGION_AU915"
DEFINES += -D"REGION_US915"
DEFINES += -D"REGION_KR920"
DEFINES += -D"REGION_US868"
DEFINES += -D"REGION_IN865"
DEFINES += -DLORAMAC_CLASSA_ENABLED
DEFINES += -DLORAMAC_CLASSB_ENABLED
DEFINES += -DLORAMAC_CLASSC_ENABLED
