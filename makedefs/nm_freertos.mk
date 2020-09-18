all:

SDKROOT?=..
include $(SDKROOT)/makedefs/nm_common.mk

ifndef FREERTOS
  FREERTOS  := $(SDKROOT)/../FreeRTOS/FreeRTOS
  $(warning FreeRTOS source location FREERTOS not defined )
  ifneq "$(wildcard $(FREERTOS) )" ""
    $(warning found FreeRTOS at $(FREERTOS))
  else
    $(error unable to locate FreeRTOS source)
  endif
endif
