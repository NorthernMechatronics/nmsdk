all:

SDKROOT?=..
include $(SDKROOT)/makedefs/nm_freertos.mk

ifndef FREERTOS-PLUS
  FREERTOS-PLUS  := $(SDKROOT)/../FreeRTOS/FreeRTOS-Plus
  $(warning FreeRTOS Plus source location FREERTOS-PLUS not defined )
  ifneq "$(wildcard $(FREERTOS-PLUS) )" ""
    $(warning found FreeRTOS at $(FREERTOS-PLUS))
  else
    $(error unable to locate FreeRTOS Plus source)
  endif

endif
