SDKROOT?=..
include $(SDKROOT)/makedefs/nm_common.mk

DEFINES += -D"REGION_AS923"
DEFINES += -D"REGION_AU915"
DEFINES += -D"REGION_EU868"
DEFINES += -D"REGION_US915"
DEFINES += -D"REGION_KR920"
DEFINES += -D"REGION_IN865"
DEFINES += -DLORAMAC_CLASSB_ENABLED
DEFINES += -DSOFT_SE
DEFINES += -DCONTEXT_MANAGEMENT_ENABLED