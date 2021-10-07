SDKROOT?=../..
include $(SDKROOT)/makedefs/nm_freertos.mk
include $(SDKROOT)/makedefs/nm_cordio.mk
include $(SDKROOT)/makedefs/nm_uecc.mk

ifdef DEBUG
    TARGET  := libcordio-dev
    CONFIG  := ./debug
else
    TARGET  := libcordio
    CONFIG  := ./release
endif
COMPILERNAME := gcc
PROJECT      := libcordio_gcc

INCLUDES  = -I$(AMBIQ_SDK)/mcu/apollo3
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/AmbiqMicro/Include
INCLUDES += -I$(AMBIQ_SDK)/CMSIS/ARM/Include
INCLUDES += -I$(AMBIQ_SDK)/utils

INCLUDES += -I$(FREERTOS)/include
INCLUDES += -I$(SDKROOT)/features/FreeRTOS

INCLUDES += -I$(CORDIO_STACK)/include
INCLUDES += -I$(CORDIO_STACK)/sources/hci/ambiq
INCLUDES += -I$(CORDIO_STACK)/sources/hci/ambiq/apollo3

INCLUDES += -I$(CORDIO_STACK)/sources/sec/common
INCLUDES += -I$(CORDIO_STACK)/sources/sec/uecc
INCLUDES += -I$(UECC)

INCLUDES += -I$(CORDIO_STACK)/sources/stack/att
INCLUDES += -I$(CORDIO_STACK)/sources/stack/cfg
INCLUDES += -I$(CORDIO_STACK)/sources/stack/dm
INCLUDES += -I$(CORDIO_STACK)/sources/stack/hci
INCLUDES += -I$(CORDIO_STACK)/sources/stack/l2c
INCLUDES += -I$(CORDIO_STACK)/sources/stack/smp

INCLUDES += -I$(CORDIO_WSF)/include
INCLUDES += -I$(CORDIO_WSF)/sources
INCLUDES += -I$(CORDIO_WSF)/sources/port/freertos
INCLUDES += -I$(CORDIO_WSF)/sources/util

INCLUDES += -I$(CORDIO_PROFILES)/include/app
INCLUDES += -I$(CORDIO_PROFILES)/sources/services
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/anpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/atpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/atps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/bas
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/blpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/blps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/cpp
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/cscp
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/dis
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/fmpl
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/gap
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/gatt
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/glpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/glps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/hid
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/hrpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/hrps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/htpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/htps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/include
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/paspc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/plxpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/plxps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/rscp
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/scpps
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/sensor
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/tipc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/udsc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/uribeacon
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/wdxc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/wdxs
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/wpc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/wspc
INCLUDES += -I$(CORDIO_PROFILES)/sources/profiles/wsps

INCLUDES += -I$(CORDIO_PROFILES)/sources/apps
INCLUDES += -I$(CORDIO_PROFILES)/sources/apps/app
INCLUDES += -I$(CORDIO_PROFILES)/sources/apps/app/common

INCLUDES += -I.

VPATH  = $(CORDIO_STACK)/sources/hci/ambiq
VPATH += $(CORDIO_STACK)/sources/hci/ambiq/apollo3

VPATH += $(CORDIO_STACK)/sources/sec/common
VPATH += $(CORDIO_STACK)/sources/sec/uecc
VPATH += $(UECC)

VPATH += $(CORDIO_STACK)/sources/stack/att
VPATH += $(CORDIO_STACK)/sources/stack/cfg
VPATH += $(CORDIO_STACK)/sources/stack/dm
VPATH += $(CORDIO_STACK)/sources/stack/hci
VPATH += $(CORDIO_STACK)/sources/stack/l2c
VPATH += $(CORDIO_STACK)/sources/stack/smp

VPATH += $(CORDIO_WSF)/sources/port/freertos
VPATH += $(CORDIO_WSF)/sources/util

VPATH += $(CORDIO_PROFILES)/sources/profiles

VPATH += $(CORDIO_PROFILES)/sources/profiles/anpc
VPATH += $(CORDIO_PROFILES)/sources/profiles/atpc
VPATH += $(CORDIO_PROFILES)/sources/profiles/atps
VPATH += $(CORDIO_PROFILES)/sources/profiles/bas
VPATH += $(CORDIO_PROFILES)/sources/profiles/blpc
VPATH += $(CORDIO_PROFILES)/sources/profiles/blps
VPATH += $(CORDIO_PROFILES)/sources/profiles/cpp
VPATH += $(CORDIO_PROFILES)/sources/profiles/cscp
VPATH += $(CORDIO_PROFILES)/sources/profiles/dis
VPATH += $(CORDIO_PROFILES)/sources/profiles/fmpl
VPATH += $(CORDIO_PROFILES)/sources/profiles/gap
VPATH += $(CORDIO_PROFILES)/sources/profiles/gatt
VPATH += $(CORDIO_PROFILES)/sources/profiles/glpc
VPATH += $(CORDIO_PROFILES)/sources/profiles/glps
VPATH += $(CORDIO_PROFILES)/sources/profiles/hid
VPATH += $(CORDIO_PROFILES)/sources/profiles/hrpc
VPATH += $(CORDIO_PROFILES)/sources/profiles/hrps
VPATH += $(CORDIO_PROFILES)/sources/profiles/htpc
VPATH += $(CORDIO_PROFILES)/sources/profiles/htps
VPATH += $(CORDIO_PROFILES)/sources/profiles/paspc
VPATH += $(CORDIO_PROFILES)/sources/profiles/plxpc
VPATH += $(CORDIO_PROFILES)/sources/profiles/plxps
VPATH += $(CORDIO_PROFILES)/sources/profiles/rscp
VPATH += $(CORDIO_PROFILES)/sources/profiles/scpps
VPATH += $(CORDIO_PROFILES)/sources/profiles/sensor
VPATH += $(CORDIO_PROFILES)/sources/profiles/tipc
VPATH += $(CORDIO_PROFILES)/sources/profiles/udsc
VPATH += $(CORDIO_PROFILES)/sources/profiles/uribeacon
VPATH += $(CORDIO_PROFILES)/sources/profiles/wdxc
VPATH += $(CORDIO_PROFILES)/sources/profiles/wdxs
VPATH += $(CORDIO_PROFILES)/sources/profiles/wpc
VPATH += $(CORDIO_PROFILES)/sources/profiles/wspc
VPATH += $(CORDIO_PROFILES)/sources/profiles/wsps

VPATH += $(CORDIO_PROFILES)/sources/apps/app
VPATH += $(CORDIO_PROFILES)/sources/apps/app/common

VPATH += $(CORDIO_PROFILES)/sources/services

VPATH += .

SRC  = hci_core.c
SRC += hci_drv_apollo3.c

SRC += hci_cmd.c
SRC += hci_cmd_ae.c
SRC += hci_cmd_cte.c
SRC += hci_cmd_past.c
SRC += hci_cmd_phy.c
SRC += hci_core.c
SRC += hci_core_ps.c
SRC += hci_evt.c
SRC += hci_tr.c
SRC += hci_vs_apollo3.c
SRC += hci_vs_ae.c

SRC += sec_aes.c
SRC += sec_aes_rev.c
SRC += sec_ccm_hci.c
SRC += sec_cmac_hci.c
SRC += sec_ecc_debug.c
SRC += sec_ecc_hci.c
SRC += sec_main.c

SRC += sec_ecc.c

SRC += attc_disc.c
SRC += attc_main.c
SRC += attc_proc.c
SRC += attc_read.c
SRC += attc_sign.c
SRC += attc_write.c
SRC += atts_ccc.c
SRC += atts_csf.c
SRC += atts_dyn.c
SRC += atts_ind.c
SRC += atts_main.c
SRC += atts_proc.c
SRC += atts_read.c
SRC += atts_sign.c
SRC += atts_write.c
SRC += att_main.c
SRC += att_uuid.c

SRC += cfg_stack.c

SRC += dm_adv.c
SRC += dm_adv_ae.c
SRC += dm_adv_leg.c
SRC += dm_conn.c
SRC += dm_conn_cte.c
SRC += dm_conn_master.c
SRC += dm_conn_master_ae.c
SRC += dm_conn_master_leg.c
SRC += dm_conn_slave.c
SRC += dm_conn_slave_ae.c
SRC += dm_conn_slave_leg.c
SRC += dm_conn_sm.c
SRC += dm_dev.c
SRC += dm_dev_priv.c
SRC += dm_main.c
SRC += dm_past.c
SRC += dm_phy.c
SRC += dm_priv.c
SRC += dm_scan.c
SRC += dm_scan_ae.c
SRC += dm_scan_leg.c
SRC += dm_sec.c
SRC += dm_sec_lesc.c
SRC += dm_sec_master.c
SRC += dm_sec_slave.c
SRC += dm_sync_ae.c

SRC += hci_main.c

SRC += l2c_coc.c
SRC += l2c_main.c
SRC += l2c_master.c
SRC += l2c_slave.c

SRC += smpi_act.c
SRC += smpi_sc_act.c
SRC += smpi_sc_sm.c
SRC += smpi_sm.c
SRC += smpr_act.c
SRC += smpr_sc_act.c
SRC += smpr_sc_sm.c
SRC += smpr_sm.c
SRC += smp_act.c
SRC += smp_db.c
SRC += smp_main.c
SRC += smp_non.c
SRC += smp_sc_act.c
SRC += smp_sc_main.c

SRC += svc_batt.c
SRC += svc_bps.c
SRC += svc_core.c
SRC += svc_cps.c
SRC += svc_cscs.c
SRC += svc_dis.c
SRC += svc_gls.c
SRC += svc_gyro.c
SRC += svc_hid.c
SRC += svc_hrs.c
SRC += svc_hts.c
SRC += svc_ipss.c
SRC += svc_plxs.c
SRC += svc_px.c
SRC += svc_rscs.c
SRC += svc_scpss.c
SRC += svc_temp.c
SRC += svc_uricfg.c
SRC += svc_wdxs.c
SRC += svc_wp.c
SRC += svc_wss.c



SRC += wsf_assert.c
SRC += wsf_buf.c
SRC += wsf_efs.c
SRC += wsf_math.c
SRC += wsf_msg.c
SRC += wsf_os.c
SRC += wsf_queue.c
SRC += wsf_timer.c
SRC += wsf_trace.c

SRC += bda.c
SRC += bstream.c
SRC += calc128.c
SRC += crc32.c
SRC += print.c
SRC += terminal.c
SRC += wstr.c

SRC += anpc_main.c
SRC += atpc_main.c
SRC += atps_main.c
SRC += bas_main.c
SRC += blpc_main.c
SRC += blps_main.c
SRC += cpps_main.c
SRC += cscps_main.c
SRC += dis_main.c
SRC += fmpl_main.c
SRC += gap_main.c
SRC += gatt_main.c
SRC += glpc_main.c
SRC += glps_main.c
SRC += glps_db.c
SRC += hid_main.c
SRC += hrpc_main.c
SRC += hrps_main.c
SRC += htpc_main.c
SRC += htps_main.c
SRC += paspc_main.c
SRC += plxpc_main.c
SRC += plxps_main.c
SRC += plxps_db.c
SRC += rscps_main.c
SRC += scpps_main.c
SRC += gyro_main.c
SRC += temp_main.c
SRC += tipc_main.c
SRC += udsc_main.c
SRC += uricfg_main.c
SRC += wdxc_main.c
SRC += wdxc_ft.c
SRC += wdxc_stream.c
SRC += wdxs_main.c
SRC += wdxs_au.c
SRC += wdxs_dc.c
SRC += wdxs_ft.c
SRC += wdxs_phy.c
SRC += wdxs_stream.c
SRC += wpc_main.c
SRC += wspc_main.c
SRC += wsps_main.c

SRC += app_disc.c
SRC += app_main.c
SRC += app_master.c
SRC += app_master_ae.c
SRC += app_master_leg.c
SRC += app_server.c
SRC += app_slave.c
SRC += app_slave_ae.c
SRC += app_slave_leg.c
SRC += app_terminal.c

SRC += app_db.c
SRC += app_hw.c
SRC += app_ui.c
SRC += ui_console.c
SRC += ui_lcd.c
SRC += ui_main.c
SRC += ui_platform.c
SRC += ui_timer.c

SRC += uECC.c
SRC += uECC_ll.c

DEFINES += -DSEC_ECC_CFG=SEC_ECC_CFG_UECC
DEFINES += -DAM_CUSTOM_BDADDR
#ifdef DEBUG
#    DEFINES += -DAM_DEBUG_PRINTF
#    DEFINES += -DWSF_TRACE_ENABLED
#endif

CSRC = $(filter %.c, $(SRC))
ASRC = $(filter %.s, $(SRC))

OBJS  = $(CSRC:%.c=$(CONFIG)/%.o)
OBJS += $(ASRC:%.s=$(CONFIG)/%.o)

DEPS  = $(CSRC:%.c=$(CONFIG)/%.d)
DEPS += $(ASRC:%.s=$(CONFIG)/%.d)

CFLAGS += $(INCLUDES)
CFLAGS += $(DEFINES)
CFLAGS += -fomit-frame-pointer

all: directories $(CONFIG)/$(TARGET).a

directories: $(CONFIG)

$(CONFIG):
	@$(MKDIR) $@

$(CONFIG)/%.o: %.c $(CONFIG)/%.d $(INCS)
	@echo "Compiling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/%.o: %.s $(CONFIG)/%.d $(INCS)
	@echo "Assembling $(COMPILERNAME) $<"
	$(CC) -c $(CFLAGS) $< -o $@

$(CONFIG)/$(TARGET).a: $(OBJS)
	$(AR) rsvc $@ $(OBJS)

clean:
	@echo "Cleaning..."
	$(RM) -f $(OBJS) $(DEPS) $(CONFIG)/$(TARGET).a
	$(RM) -rf $(CONFIG)

$(CONFIG)/%.d: ;

-include $(DEPS)
