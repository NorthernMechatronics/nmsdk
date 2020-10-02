/******************************************************************************
 *
 * Filename   : loramac_app.c
 * Description  : LoRaMac task implementation
 *
 ******************************************************************************/

/******************************************************************************
 * Standard header files
 ******************************************************************************/
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/******************************************************************************
 * Ambiq Micro header files
 ******************************************************************************/
#include <am_bsp.h>
#include <am_mcu_apollo.h>
#include <am_util.h>

/******************************************************************************
 * LoRaMac-node header files
 ******************************************************************************/
#include <LoRaMac.h>
#include <LoRaMacTest.h>

/******************************************************************************
 * LoRaMac-node adaptation header files
 ******************************************************************************/
#include <board.h>
#include <gpio.h>
#include <utilities.h>

/******************************************************************************
 * Application header files
 ******************************************************************************/
#include "loramac_app.h"

#include "loramac_compliance.h"

/******************************************************************************
 * Macros
 ******************************************************************************/
#define APP_TX_NEXT_TIMEOUT (5000)
#define APP_TX_NEXT_TIMEOUT_RND (1000)
#define MAX_APP_TX_QUEUE_SIZE (3)
#define MAX_APP_BUFFERS (3)

/******************************************************************************
 * Type definitions
 ******************************************************************************/
typedef struct
{
    bool queued;
    bool sent;
    uint8_t *buf;
    uint8_t len;
    loramac_app_buf_done_fn done;

    bool ack_mode;
    uint8_t port;

    uint8_t next;
} loramac_app_data_s;

typedef struct
{
    bool available;
    uint8_t buf[MAX_APP_DATA_SIZE];
} loramac_app_buffer_s;

/******************************************************************************
 * Local declarations
 ******************************************************************************/
static loramac_compliance_status_t loramac_compliance_status;

static loramac_app_state_e app_state = APP_STATE_NULL;
static DeviceClass_t app_class = CLASS_A;
static loramac_app_timing_e app_timing = APP_TIMING_DEVICE;

static uint32_t loramac_app_tx_timeout = APP_TX_NEXT_TIMEOUT;
static uint32_t loramac_app_tx_timeout_rnd = APP_TX_NEXT_TIMEOUT_RND;

static LoRaMacPrimitives_t loramac_evt_fn;
static LoRaMacCallback_t loramac_cb;

static uint8_t loramac_channel_mask_count = 5;
static bool loramac_duty_cycle_used = false;

static uint32_t next_tx_time;
static TimerEvent_t next_tx_timer;

static uint8_t app_can_sleep = false;
static loramac_app_buffer_s app_buf[MAX_APP_BUFFERS];

static loramac_app_data_s ul_data_queue[MAX_APP_TX_QUEUE_SIZE];
static loramac_app_data_s *ul_data = &ul_data_queue[0];

static const char *loramac_req_stat_str[] = {"OK",
                                             "BUSY",
                                             "SERVICE_UNKNOWN",
                                             "PARAMETER_INVALID",
                                             "FREQUENCY_INVALID",
                                             "DATARATE_INVALID",
                                             "FREQ_AND_DR_INVALID",
                                             "NO_NETWORK_JOINED",
                                             "LENGTH_ERROR",
                                             "REGION_NOT_SUPPORTED",
                                             "SKIPPED_APP_DATA",
                                             "DUTYCYCLE_RESTRICTED",
                                             "NO_CHANNEL_FOUND",
                                             "NO_FREE_CHANNEL_FOUND",
                                             "BUSY_BEACON_RESERVED_TIME",
                                             "BUSY_PING_SLOT_WINDOW_TIME",
                                             "BUSY_UPLINK_COLLISION",
                                             "CRYPTO_ERROR",
                                             "FCNT_HANDLER_ERROR",
                                             "MAC_COMMAD_ERROR",
                                             "CLASS_B_ERROR",
                                             "CONFIRM_QUEUE_ERROR",
                                             "MC_GROUP_UNDEFINED",
                                             "ERROR"};

static const char *loramac_mlstat_str[] = {"OK",
                                           "ERROR",
                                           "TX_TIMEOUT",
                                           "RX1_TIMEOUT",
                                           "RX2_TIMEOUT",
                                           "RX1_ERROR",
                                           "RX2_ERROR",
                                           "JOIN_FAIL",
                                           "DOWNLINK_REPEATED",
                                           "TX_DR_PAYLOAD_SIZE_ERROR",
                                           "DOWNLINK_TOO_MANY_FRAMES_LOSS",
                                           "ADDRESS_FAIL",
                                           "MIC_FAIL",
                                           "MULTICAST_FAIL",
                                           "BEACON_LOCKED",
                                           "BEACON_LOST",
                                           "BEACON_NOT_FOUND"};

/******************************************************************************
 * Local functions
 ******************************************************************************
 * s_dump_hex
 * ----------
 * Print out the buffer as hexadecimal bytes with proper spacing
 *
 * Parameters :
 *    @prefix - Prefix for each full line
 *    @buf    - Buffer to print
 *    @len    - Length of buffer
 *    @sp     - Spacing character
 *
 ******************************************************************************/
static void s_dump_hex(char *prefix, uint8_t *buf, uint8_t len, char sp)
{
    uint8_t width = (len < 16) ? len : 16;

    for (uint8_t i = 0; i < len; i++)
    {
        if ((i % width) == 0 && prefix)
        {
            am_util_debug_printf(prefix);
        }
        if (((i + 1) % width) == 0)
        {
            am_util_debug_printf("%02X\n", buf[i]);
        }
        else
        {
            am_util_debug_printf("%02X%c", buf[i], sp);
        }
    }
    if (len % width != 0)
    {
        am_util_debug_printf("\n");
    }
}

/******************************************************************************
 * s_join_network
 * ------------
 * Join the network
 *
 ******************************************************************************/
static void s_join_network(void)
{
    LoRaMacStatus_t status;
    MlmeReq_t mlme_req;

    mlme_req.Type = MLME_JOIN;
    mlme_req.Req.Join.Datarate = DEFAULT_LORAMAC_DATARATE;
    status = LoRaMacMlmeRequest(&mlme_req);
    am_util_debug_printf("\n###### ===== MLME-Request - MLME_JOIN ==== ######\n");
    am_util_debug_printf("STATUS      : %s\n\n", loramac_req_stat_str[status]);

    if (status == LORAMAC_STATUS_OK)
    {
        am_util_debug_printf("###### ===== JOINING ==== ######\n");
        app_state = APP_STATE_SLEEP;
    }
    else
    {
        if (status == LORAMAC_STATUS_DUTYCYCLE_RESTRICTED)
        {
            am_util_debug_printf("Next Tx in  : %lu [ms]\n", mlme_req.ReqReturn.DutyCycleWaitTime);
        }
        app_state = APP_STATE_CYCLE;
    }
}

/******************************************************************************
 * s_loramac_send_frame
 * --------------------
 * Send data
 *
 * Returns  :
 *    TRUE, if packet send needs to be scheduled later.  FALSE, otherwise.
 ******************************************************************************/
static bool s_loramac_send_frame(void)
{
    LoRaMacStatus_t status;
    McpsReq_t mcps_req;
    LoRaMacTxInfo_t tx_info;
    bool reschedule = false;

    if (LoRaMacQueryTxPossible(ul_data->len, &tx_info) != LORAMAC_STATUS_OK)
    {
        am_util_debug_printf("\n###### ====== MAC Flush ====== ######\n");
        mcps_req.Type = MCPS_UNCONFIRMED;
        mcps_req.Req.Unconfirmed.fBuffer = NULL;
        mcps_req.Req.Unconfirmed.fBufferSize = 0;
        mcps_req.Req.Unconfirmed.Datarate = DEFAULT_LORAMAC_DATARATE;
        reschedule = true;
    }
    else
    {
        if (ul_data->ack_mode)
        {
            mcps_req.Type = MCPS_CONFIRMED;
            mcps_req.Req.Confirmed.fPort = ul_data->port;
            mcps_req.Req.Confirmed.fBuffer = ul_data->buf;
            mcps_req.Req.Confirmed.fBufferSize = ul_data->len;
            mcps_req.Req.Confirmed.NbTrials = 8;
            mcps_req.Req.Confirmed.Datarate = DEFAULT_LORAMAC_DATARATE;
        }
        else
        {
            mcps_req.Type = MCPS_UNCONFIRMED;
            mcps_req.Req.Unconfirmed.fPort = ul_data->port;
            mcps_req.Req.Unconfirmed.fBuffer = ul_data->buf;
            mcps_req.Req.Unconfirmed.fBufferSize = ul_data->len;
            mcps_req.Req.Unconfirmed.Datarate = DEFAULT_LORAMAC_DATARATE;
        }
        ul_data->sent = true;
    }

    status = LoRaMacMcpsRequest(&mcps_req);
    am_util_debug_printf("\n###### ===== MCPS-Request ==== ######\n");
    am_util_debug_printf("STATUS      : %s\n", loramac_req_stat_str[status]);

    if (status == LORAMAC_STATUS_OK)
    {
        return reschedule;
    }

    if (status == LORAMAC_STATUS_DUTYCYCLE_RESTRICTED)
    {
        am_util_debug_printf("Next Tx in  : %lu [ms]\n", mcps_req.ReqReturn.DutyCycleWaitTime);
    }

    if (status != LORAMAC_STATUS_OK)
    {
        return false;
    }

    return true;
}

/******************************************************************************
 * next_tx_expiry_fn
 * -----------------
 * Timer-expiry handler
 *
 * Parameters   :
 *    @context  - Not used
 *
 ******************************************************************************/
static void next_tx_expiry_fn(void *context)
{
    MibRequestConfirm_t mib_req;
    LoRaMacStatus_t status;

    TimerStop(&next_tx_timer);

    mib_req.Type = MIB_NETWORK_ACTIVATION;
    status = LoRaMacMibGetRequestConfirm(&mib_req);

    if (status == LORAMAC_STATUS_OK)
    {
        if (mib_req.Param.NetworkActivation == ACTIVATION_TYPE_NONE)
        {
            s_join_network();
        }
        else
        {
            app_state = APP_STATE_SEND;
        }
    }
}

/******************************************************************************
 * s_mcps_cnf
 * ----------
 * Handle MCPS confirmation message
 *
 * Parameters   :
 *    @mcps_cnf - Confirmation message contents
 *
 ******************************************************************************/
static void s_mcps_cnf(McpsConfirm_t *mcps_cnf)
{
    MibRequestConfirm_t mib_get;
    MibRequestConfirm_t mib_req;

    am_util_debug_printf("\n###### ===== MCPS-Confirm ==== ######\n");
    am_util_debug_printf("STATUS      : %s\n", loramac_mlstat_str[mcps_cnf->Status]);
    if (mcps_cnf->Status == LORAMAC_EVENT_INFO_STATUS_OK)
    {
        switch (mcps_cnf->McpsRequest)
        {
        case MCPS_UNCONFIRMED:
            break;

        case MCPS_CONFIRMED:
            break;

        case MCPS_PROPRIETARY:
            break;

        default:
            break;
        }
    }

    mib_req.Type = MIB_DEVICE_CLASS;
    LoRaMacMibGetRequestConfirm(&mib_req);

    am_util_debug_printf("\n###### ===== UPLINK FRAME %lu ==== ######\n", mcps_cnf->UpLinkCounter);
    am_util_debug_printf("CLASS       : %c\n", "ABC"[mib_req.Param.Class]);
    am_util_debug_printf("TX PORT     : %d\n", ul_data->port);

    if (ul_data->len != 0 && ul_data->sent)
    {
        am_util_debug_printf("TX DATA     : ");
        if (ul_data->ack_mode)
        {
            am_util_debug_printf("CONFIRMED - %s\n", (mcps_cnf->AckReceived != 0) ? "ACK" : "NACK");
        }
        else
        {
            am_util_debug_printf("UNCONFIRMED\n");
        }
        s_dump_hex("              ", ul_data->buf, ul_data->len, ' ');
    }

    am_util_debug_printf("DATA RATE   : DR_%d\n", mcps_cnf->Datarate);

    mib_get.Type = MIB_CHANNELS;
    if (LoRaMacMibGetRequestConfirm(&mib_get) == LORAMAC_STATUS_OK)
    {
        am_util_debug_printf("U/L FREQ    : %lu\n", mib_get.Param.ChannelList[mcps_cnf->Channel].Frequency);
    }

    am_util_debug_printf("TX POWER    : %d\n", mcps_cnf->TxPower);

    mib_get.Type = MIB_CHANNELS_MASK;
    if (LoRaMacMibGetRequestConfirm(&mib_get) == LORAMAC_STATUS_OK)
    {
        am_util_debug_printf("CHANNEL MASK: ");
        for (uint8_t i = 0; i < loramac_channel_mask_count; i++)
        {
            am_util_debug_printf("%04X ", mib_get.Param.ChannelsMask[i]);
        }
    }
    am_util_debug_printf("\n");
    if (ul_data->queued && ul_data->sent)
    {
        if (ul_data->done)
        {
            ul_data->done(ul_data->buf, true);
        }
        else
        {
            for (int i = 0; i < MAX_APP_BUFFERS; i++)
            {
                if (ul_data->buf >= app_buf[i].buf && ul_data->buf < app_buf[i].buf + MAX_APP_DATA_SIZE)
                {
                    app_buf[i].available = true;
                }
            }
        }
        ul_data->buf = NULL;
        ul_data->len = 0;
        ul_data->sent = false;
        ul_data->queued = false;
        ul_data->done = NULL;
        ul_data = &ul_data_queue[ul_data->next];
    }
}

/******************************************************************************
 * s_mcps_ind
 * ----------
 * MCPS-Indication event function
 *
 * Parameters   :
 *    @mcps_ind - Pointer to indication structure
 *
 ******************************************************************************/
static void s_mcps_ind(McpsIndication_t *mcps_ind)
{
    am_util_debug_printf("\n###### ===== MCPS-Indication ==== ######\n");
    am_util_debug_printf("STATUS      : %s\n", loramac_mlstat_str[mcps_ind->Status]);
    if (mcps_ind->Status != LORAMAC_EVENT_INFO_STATUS_OK)
    {
        return;
    }

    switch (mcps_ind->McpsIndication)
    {
    case MCPS_UNCONFIRMED:
        break;

    case MCPS_CONFIRMED:
        break;

    case MCPS_PROPRIETARY:
        break;

    case MCPS_MULTICAST:
        break;

    default:
        break;
    }

    if (mcps_ind->FramePending == true)
    {
        next_tx_expiry_fn(NULL);
    }

    if (loramac_compliance_status.Running == true)
    {
        loramac_compliance_status.DownLinkCounter++;
    }

    if (mcps_ind->RxData == true)
    {
        // Port 1 and 2 are used for the application layer
        switch (mcps_ind->Port)
        {
        case 1:
        case 2:
        break;

        case LORAMAC_COMPLIANCE_TEST_PORT:

            if (loramac_compliance_status.Running == false)
            {
                // Check compliance test enable command (i)
                if ((mcps_ind->BufferSize == 4) &&
                	(mcps_ind->Buffer[0] == 0x01) &&
					(mcps_ind->Buffer[1] == 0x01) &&
                    (mcps_ind->Buffer[2] == 0x01) &&
					(mcps_ind->Buffer[3] == 0x01))
                {
                    loramac_compliance_status.IsTxConfirmed = false;
                    loramac_compliance_status.AppPort = LORAMAC_COMPLIANCE_TEST_PORT;
                    loramac_compliance_status.AppDataSize = 2;
                    loramac_compliance_status.DownLinkCounter = 0;
                    loramac_compliance_status.LinkCheck = false;
                    loramac_compliance_status.DemodMargin = 0;
                    loramac_compliance_status.NbGateways = 0;
                    loramac_compliance_status.Running = true;
                    loramac_compliance_status.State = COMPLIANCE_ACTIVE;

                    MibRequestConfirm_t mib_req;
                    mib_req.Type = MIB_ADR;
                    mib_req.Param.AdrEnable = true;
                    LoRaMacMibSetRequestConfirm(&mib_req);

                    // turn off duty cycle during testing
                    if (loramac_duty_cycle_used)
                    {
                        LoRaMacTestSetDutyCycleOn(false);
                    }
                }
            }
            else
            {
                loramac_compliance_status.State = mcps_ind->Buffer[0];
                switch (loramac_compliance_status.State)
                {
                case COMPLIANCE_EXIT: {
                    loramac_compliance_status.DownLinkCounter = 0;
                    loramac_compliance_status.Running = false;

                    MibRequestConfirm_t mib_req;
                    mib_req.Type = MIB_ADR;
                    mib_req.Param.AdrEnable = true;
                    LoRaMacMibSetRequestConfirm(&mib_req);

                    if (loramac_duty_cycle_used)
                    {
                        LoRaMacTestSetDutyCycleOn(true);
                    }
                }
                break;

                case COMPLIANCE_ACTIVE:
                    loramac_compliance_status.AppDataSize = 2;
                    break;

                case COMPLIANCE_ACK_ENABLE:
                    loramac_compliance_status.IsTxConfirmed = true;
                    loramac_compliance_status.State = COMPLIANCE_ACTIVE;
                    break;

                case COMPLIANCE_ACK_DISABLE:
                    loramac_compliance_status.IsTxConfirmed = false;
                    loramac_compliance_status.State = COMPLIANCE_ACTIVE;
                    break;

                case COMPLIANCE_RX:
                    loramac_compliance_status.AppDataSize = mcps_ind->BufferSize;
                    loramac_compliance_status.AppDataBuffer[0] = 4;
                    for (uint8_t i = 1; i < MIN(loramac_compliance_status.AppDataSize, MAX_APP_DATA_SIZE); i++)
                    {
                        loramac_compliance_status.AppDataBuffer[i] = mcps_ind->Buffer[i] + 1;
                    }
                    break;

                case COMPLIANCE_LINK_CHECK: {
                    MlmeReq_t mlme_req;
                    mlme_req.Type = MLME_LINK_CHECK;
                    LoRaMacStatus_t status = LoRaMacMlmeRequest(&mlme_req);
                    am_util_debug_printf("\n###### ===== MLME-Request - MLME_LINK_CHECK ==== ######\n");
                    am_util_debug_printf("STATUS      : %s\n", loramac_req_stat_str[status]);
                }
                break;

                case COMPLIANCE_EXIT_AND_REJOIN: {

                    loramac_compliance_status.DownLinkCounter = 0;
                    loramac_compliance_status.Running = false;

                    MibRequestConfirm_t mib_req;
                    mib_req.Type = MIB_ADR;
                    mib_req.Param.AdrEnable = true;
                    LoRaMacMibSetRequestConfirm(&mib_req);

                    if (loramac_duty_cycle_used)
                    {
                        LoRaMacTestSetDutyCycleOn(true);
                    }

                    s_join_network();
                }
                break;

                case COMPLIANCE_TXCW: {
                    if (mcps_ind->BufferSize == 3)
                    {
                        MlmeReq_t mlme_req;
                        mlme_req.Type = MLME_TXCW;
                        mlme_req.Req.TxCw.Timeout = (uint16_t)((mcps_ind->Buffer[1] << 8) | mcps_ind->Buffer[2]);
                        LoRaMacStatus_t status = LoRaMacMlmeRequest(&mlme_req);
                        am_util_debug_printf("\n###### ===== MLME-Request - MLME_TXCW ==== ######\n");
                        am_util_debug_printf("STATUS      : %s\n", loramac_req_stat_str[status]);
                    }
                    else if (mcps_ind->BufferSize == 7)
                    {
                        MlmeReq_t mlme_req;
                        mlme_req.Type = MLME_TXCW_1;
                        mlme_req.Req.TxCw.Timeout = (uint16_t)((mcps_ind->Buffer[1] << 8) | mcps_ind->Buffer[2]);
                        mlme_req.Req.TxCw.Frequency = (uint32_t)((mcps_ind->Buffer[3] << 16) |
                                                                 (mcps_ind->Buffer[4] << 8) | mcps_ind->Buffer[5]) *
                                                      100;
                        mlme_req.Req.TxCw.Power = mcps_ind->Buffer[6];
                        LoRaMacStatus_t status = LoRaMacMlmeRequest(&mlme_req);
                        am_util_debug_printf("\n###### ===== MLME-Request - MLME_TXCW1 ==== ######\n");
                        am_util_debug_printf("STATUS      : %s\n", loramac_req_stat_str[status]);
                    }
                    loramac_compliance_status.State = COMPLIANCE_ACTIVE;
                }
                break;

                case COMPLIANCE_REQ_DEVICE_TIME: {
                    MlmeReq_t mlme_req;
                    mlme_req.Type = MLME_DEVICE_TIME;
                    LoRaMacStatus_t status = LoRaMacMlmeRequest(&mlme_req);
                    am_util_debug_printf("\n###### ===== MLME-Request - MLME_DEVICE_TIME ==== ######\n");
                    am_util_debug_printf("STATUS      : %s\n", loramac_req_stat_str[status]);
                    app_state = APP_STATE_SEND;
                }
                break;

                case COMPLIANCE_SWITCH_CLASS: {
                	MibRequestConfirm_t mib_req;

                	mib_req.Type = MIB_DEVICE_CLASS;
                	mib_req.Param.Class = (DeviceClass_t)mcps_ind->Buffer[1];
                	LoRaMacMibSetRequestConfirm(&mib_req);

                	app_state = APP_STATE_SEND;
                }
                break;

                case COMPLIANCE_REQ_PING_SLOT_INFO: {
                    MlmeReq_t mib_req;

                    mib_req.Type = MLME_PING_SLOT_INFO;

                    mib_req.Req.PingSlotInfo.PingSlot.Value = mcps_ind->Buffer[1];

                    LoRaMacMlmeRequest( &mib_req );
                    app_state = APP_STATE_SEND;
                    app_state = APP_STATE_SEND;
                }
                break;

                case COMPLIANCE_REQ_BEACON_TIMING: {
					MlmeReq_t mlme_req;

					mlme_req.Type = MLME_BEACON_TIMING;

					LoRaMacMlmeRequest( &mlme_req );
					app_state = APP_STATE_SEND;
				}
				break;

                default:
                    break;
                }
            }
        break;
        }
    }

    const char *slotStrings[] = {"1", "2", "C", "C Multicast", "B Ping-Slot", "B Multicast Ping-Slot"};

    am_util_debug_printf("\n###### ===== DOWNLINK FRAME %lu ==== ######\n", mcps_ind->DownLinkCounter);
    am_util_debug_printf("RX WINDOW   : %s\n", slotStrings[mcps_ind->RxSlot]);
    am_util_debug_printf("RX PORT     : %d\n", mcps_ind->Port);
    if (mcps_ind->BufferSize != 0)
    {
        am_util_debug_printf("RX DATA   : ");
        if (mcps_ind->McpsIndication == MCPS_UNCONFIRMED)
        {
            am_util_debug_printf("UNCONFIRMED\n");
        }
        else if (mcps_ind->McpsIndication == MCPS_CONFIRMED)
        {
            am_util_debug_printf("CONFIRMED\n");
        }
        else if (mcps_ind->McpsIndication == MCPS_MULTICAST)
        {
            am_util_debug_printf("MULTICAST\n");
        }
        else if (mcps_ind->McpsIndication == MCPS_PROPRIETARY)
        {
            am_util_debug_printf("PROPRIETARY\n");
        }
        s_dump_hex("              ", mcps_ind->Buffer, mcps_ind->BufferSize, ' ');
    }
    am_util_debug_printf("DATA RATE   : DR_%d\n", mcps_ind->RxDatarate);
    am_util_debug_printf("RX RSSI     : %d\n", mcps_ind->Rssi);
    am_util_debug_printf("RX SNR      : %d\n", mcps_ind->Snr);
}

/******************************************************************************
 * s_mlme_cnf
 * ----------
 * MLME-Confirm event function
 *
 * Parameters :
 *    @mlme_cnf - Pointer to the confirm structure
 *
 ******************************************************************************/
static void s_mlme_cnf(MlmeConfirm_t *mlme_cnf)
{
    am_util_debug_printf("\n###### ===== MLME-Confirm ==== ######\n");
    am_util_debug_printf("STATUS      : %s\n\n", loramac_mlstat_str[mlme_cnf->Status]);
    switch (mlme_cnf->MlmeRequest)
    {
    case MLME_JOIN: {
        if (mlme_cnf->Status == LORAMAC_EVENT_INFO_STATUS_OK)
        {
            MibRequestConfirm_t mib_get;
            am_util_debug_printf("###### ===== JOINED ==== ######\n");
            am_util_debug_printf("AUTH TYPE   : OTAA\n");

            mib_get.Type = MIB_DEV_ADDR;
            LoRaMacMibGetRequestConfirm(&mib_get);
            am_util_debug_printf("DevAddr     : %08lX\n", mib_get.Param.DevAddr);

            mib_get.Type = MIB_CHANNELS_DATARATE;
            LoRaMacMibGetRequestConfirm(&mib_get);
            am_util_debug_printf("DATA RATE   : DR_%d\n", mib_get.Param.ChannelsDatarate);

            if (app_class == CLASS_B)
            {
            	if (app_timing == APP_TIMING_BEACON)
            	{
            		app_state = APP_STATE_REQ_BEACON_TIMING;
            	}
            	else
            	{
            		app_state = APP_STATE_REQ_DEVICE_TIME;
            	}
            }
            else
            {
            	app_state = APP_STATE_SEND;
            }
        }
        else
        {
            s_join_network();
        }
    }
    break;

    case MLME_LINK_CHECK: {
        if (mlme_cnf->Status == LORAMAC_EVENT_INFO_STATUS_OK)
        {
            if (loramac_compliance_status.Running == true)
            {
                loramac_compliance_status.LinkCheck = true;
                loramac_compliance_status.DemodMargin = mlme_cnf->DemodMargin;
                loramac_compliance_status.NbGateways = mlme_cnf->NbGateways;
            }
        }
    }
    break;

    case MLME_DEVICE_TIME: {
    	// TODO
    }
    break;

    case MLME_BEACON_TIMING: {
    	// TODO
    }
    break;

    case MLME_BEACON_ACQUISITION: {
    	// TODO
    }
    break;

    case MLME_PING_SLOT_INFO: {
    	// TODO
    }
    break;

    default:
        break;
    }
}

/******************************************************************************
 * s_mlme_ind
 * ----------
 * MLME-Indication event function
 *
 * Parameters   :
 *    @mlme_ind - Pointer to the indication structure
 *
 ******************************************************************************/
static void s_mlme_ind(MlmeIndication_t *mlme_ind)
{
    if (mlme_ind->Status != LORAMAC_EVENT_INFO_STATUS_BEACON_LOCKED)
    {
        am_util_debug_printf("\n###### ===== MLME-Indication ==== ######\n");
        am_util_debug_printf("STATUS      : %s\n", loramac_mlstat_str[mlme_ind->Status]);
    }
    switch (mlme_ind->MlmeIndication)
    {
    case MLME_SCHEDULE_UPLINK:
        next_tx_expiry_fn(NULL);
        break;

    case MLME_BEACON_LOST:
    	// TODO
    	break;

    case MLME_BEACON:
    	// TODO
    	break;

    default:
        break;
    }
}

/******************************************************************************
 * s_on_mac_process_notify
 * -----------------------
 * Handle MAC processing callback (ISR context)
 *
 ******************************************************************************/
static void s_on_mac_process_notify(void)
{
    app_can_sleep = false;
}

bool loramac_multicast_init(LoRaMacRegion_t region, DeviceClass_t DeviceClass,
							  uint32_t ui32MulticastAddress,
							  uint8_t *pui8AppSessionKey,
							  uint8_t *pui8NetworkSessionKey,
							  uint16_t ui8Periodicity)
{
	if (DeviceClass == CLASS_A)
	{
		return false;
	}

	//                               AS923,     AU915,     CN470,     CN779,     EU433,     EU868,     KR920,     IN865,     US915,     RU864
    const uint32_t frequencies[] = { 923200000, 923300000, 505300000, 786000000, 434665000, 869525000, 921900000, 866550000, 923300000, 869100000 };
    const int8_t dataRates[]     = { DR_2,      DR_2,      DR_0,      DR_0,      DR_0,      DR_0,      DR_0,      DR_0,      DR_0,      DR_0 };

    McChannelParams_t channel =
    {
        .IsRemotelySetup = false,
        .Class = DeviceClass,
        .IsEnabled = true,
        .GroupID = MULTICAST_0_ADDR,
        .Address = ui32MulticastAddress,
        .McKeys =
        {
            .Session =
            {
                .McAppSKey = pui8AppSessionKey,
                .McNwkSKey = pui8NetworkSessionKey,
            },
        },
        .FCountMin = 0,
        .FCountMax = UINT32_MAX,
    };

    if (DeviceClass == CLASS_B)
    {
    	channel.RxParams.ClassB.Frequency = frequencies[region];
    	channel.RxParams.ClassB.Datarate = dataRates[region];
    	channel.RxParams.ClassB.Periodicity = ui8Periodicity;
    }
    else if (DeviceClass == CLASS_C)
    {
    	channel.RxParams.ClassC.Frequency = frequencies[region];
    	channel.RxParams.ClassC.Datarate = dataRates[region];
    }

    LoRaMacStatus_t status = LoRaMacMcChannelSetup( &channel );
    if( status == LORAMAC_STATUS_OK )
    {
        uint8_t mcChannelSetupStatus = 0x00;
        if( LoRaMacMcChannelSetupRxParams( channel.GroupID, &channel.RxParams, &mcChannelSetupStatus ) == LORAMAC_STATUS_OK )
        {
            if( ( mcChannelSetupStatus & 0xFC ) == 0x00 )
            {
                am_util_debug_printf("MC #%d setup, OK\n", ( mcChannelSetupStatus & 0x03 ) );
            }
            else
            {
                am_util_debug_printf("MC #%d setup, ERROR - ", ( mcChannelSetupStatus & 0x03 ) );
                if( ( mcChannelSetupStatus & 0x10 ) == 0x10 )
                {
                    am_util_debug_printf("MC group UNDEFINED - ");
                }
                else
                {
                    am_util_debug_printf("MC group OK - ");
                }

                if( ( mcChannelSetupStatus & 0x08 ) == 0x08 )
                {
                    am_util_debug_printf("MC Freq ERROR - ");
                }
                else
                {
                    am_util_debug_printf("MC Freq OK - ");
                }
                if( ( mcChannelSetupStatus & 0x04 ) == 0x04 )
                {
                    am_util_debug_printf("MC datarate ERROR\n");
                }
                else
                {
                    am_util_debug_printf("MC datarate OK\n");
                }
            }
        }
        else
        {
        	am_util_debug_printf( "MC Rx params setup, error: %s \n", loramac_req_stat_str[status] );
        }
    }
    else
    {
    	am_util_debug_printf( "MC setup, error: %s \n", loramac_req_stat_str[status] );
    }

	return true;
}

/******************************************************************************
 * Global functions
 ******************************************************************************
 * g_loramac_init
 ******************************************************************************/
bool g_loramac_init(LoRaMacRegion_t region)
{
    LoRaMacStatus_t status;

    loramac_evt_fn.MacMcpsConfirm = s_mcps_cnf;
    loramac_evt_fn.MacMcpsIndication = s_mcps_ind;
    loramac_evt_fn.MacMlmeConfirm = s_mlme_cnf;
    loramac_evt_fn.MacMlmeIndication = s_mlme_ind;
    loramac_cb.GetBatteryLevel = NULL;
    loramac_cb.GetTemperatureLevel = NULL;
    loramac_cb.NvmContextChange = NULL;
    loramac_cb.MacProcessNotify = s_on_mac_process_notify;

    switch (region)
    {
    case LORAMAC_REGION_AS923:
        loramac_channel_mask_count = 1;
        loramac_duty_cycle_used = false;
        break;

    case LORAMAC_REGION_AU915:
        loramac_channel_mask_count = 5;
        loramac_duty_cycle_used = false;
        break;

    case LORAMAC_REGION_CN470:
        loramac_channel_mask_count = 5;
        loramac_duty_cycle_used = false;
        break;

    case LORAMAC_REGION_CN779:
        loramac_channel_mask_count = 1;
        loramac_duty_cycle_used = true;
        break;

    case LORAMAC_REGION_EU433:
        loramac_channel_mask_count = 1;
        loramac_duty_cycle_used = true;
        break;

    case LORAMAC_REGION_EU868:
        loramac_channel_mask_count = 1;
        loramac_duty_cycle_used = true;
        break;

    case LORAMAC_REGION_KR920:
        loramac_channel_mask_count = 1;
        loramac_duty_cycle_used = false;
        break;

    case LORAMAC_REGION_IN865:
        loramac_channel_mask_count = 1;
        loramac_duty_cycle_used = false;
        break;

    default: // Set US915 as default region
    case LORAMAC_REGION_US915:
        loramac_channel_mask_count = 5;
        loramac_duty_cycle_used = false;
        break;

    case LORAMAC_REGION_RU864:
        loramac_channel_mask_count = 1;
        loramac_duty_cycle_used = true;
        break;
    }

    status = LoRaMacInitialization(&loramac_evt_fn, &loramac_cb, region);
    if (status != LORAMAC_STATUS_OK)
    {
        am_util_debug_printf("LoRaMac wasn't properly initialized, error: %s", loramac_req_stat_str[status]);
        return false;
    }

    for (int i = 0; i < MAX_APP_TX_QUEUE_SIZE; i++)
    {
        ul_data_queue[i].queued = false;
        ul_data_queue[i].buf = NULL;
        ul_data_queue[i].len = 0;
        ul_data_queue[i].done = NULL;

        ul_data_queue[i].ack_mode = false;
        ul_data_queue[i].port = 0;

        ul_data_queue[i].next = (i + 1) % MAX_APP_TX_QUEUE_SIZE;
    }
    for (int i = 0; i < MAX_APP_BUFFERS; i++)
    {
        app_buf[i].available = true;
    }
    app_state = APP_STATE_START;
    return true;
}

/******************************************************************************
 * g_loramac_reset
 ******************************************************************************/
void g_loramac_reset(void)
{
    TimerStop(&next_tx_timer);
    app_state = APP_STATE_NULL;
    LoRaMacStop();
}

/******************************************************************************
 * g_loramac_state_machine
 ******************************************************************************/
loramac_app_state_e g_loramac_state_machine(void)
{
    MibRequestConfirm_t mib_req;
    LoRaMacStatus_t status;

    if (app_state == APP_STATE_NULL)
    {
        return app_state;
    }
    if (Radio.IrqProcess != NULL)
    {
        Radio.IrqProcess();
    }
    LoRaMacProcess();
    switch (app_state)
    {
    case APP_STATE_START:
        TimerInit(&next_tx_timer, next_tx_expiry_fn);

        mib_req.Type = MIB_PUBLIC_NETWORK;
        mib_req.Param.EnablePublicNetwork = true;
        LoRaMacMibSetRequestConfirm(&mib_req);

        mib_req.Type = MIB_ADR;
        mib_req.Param.AdrEnable = DEFAULT_LORAMAC_ADR_MODE;
        LoRaMacMibSetRequestConfirm(&mib_req);

        if (loramac_duty_cycle_used)
        {
            LoRaMacTestSetDutyCycleOn(true);
        }

        mib_req.Type = MIB_SYSTEM_MAX_RX_ERROR;
        mib_req.Param.SystemMaxRxError = 20;
        LoRaMacMibSetRequestConfirm(&mib_req);

        LoRaMacStart();

        mib_req.Type = MIB_NETWORK_ACTIVATION;
        status = LoRaMacMibGetRequestConfirm(&mib_req);

        if (status == LORAMAC_STATUS_OK)
        {
            if (mib_req.Param.NetworkActivation == ACTIVATION_TYPE_NONE)
            {
                app_state = APP_STATE_JOIN;
            }
            else if (ul_data->queued)
            {
                app_state = APP_STATE_SEND;
            }
            else
            {
                app_state = APP_STATE_SLEEP;
            }
        }
        break;

    case APP_STATE_JOIN:
        mib_req.Type = MIB_DEV_EUI;
        LoRaMacMibGetRequestConfirm(&mib_req);
        am_util_debug_printf("DevEUI      : ");
        s_dump_hex(NULL, mib_req.Param.DevEui, 8, '-');

        mib_req.Type = MIB_JOIN_EUI;
        LoRaMacMibGetRequestConfirm(&mib_req);
        am_util_debug_printf("JoinEUI     : ");
        s_dump_hex(NULL, mib_req.Param.JoinEui, 8, '-');

        mib_req.Type = MIB_SE_PIN;
        LoRaMacMibGetRequestConfirm(&mib_req);
        am_util_debug_printf("PIN         : ");
        s_dump_hex(NULL, mib_req.Param.SePin, 4, '-');
    	// TODO: set class B app state

        s_join_network();
        break;
    case APP_STATE_REQ_DEVICE_TIME:
    	// TODO
    	break;

    case APP_STATE_REQ_BEACON_TIMING:
    	// TODO
    	break;

    case APP_STATE_BEACON_ACQUISITION:
    	// TODO
    	break;

    case APP_STATE_REQ_PINGSLOT_ACK:
    	// TODO
    	break;

    case APP_STATE_SEND:
        if (ul_data->queued == true)
        {
            mib_req.Type = MIB_DEVICE_CLASS;
            LoRaMacMibGetRequestConfirm(&mib_req);

            if (mib_req.Param.Class != app_class)
            {
                mib_req.Param.Class = app_class;
                LoRaMacMibSetRequestConfirm(&mib_req);
            }

            if (s_loramac_send_frame())
            {
                app_state = APP_STATE_CYCLE;
            }
            else
            {
                app_state = APP_STATE_SLEEP;
            }
        }
        else
        {
            app_state = APP_STATE_SLEEP;
        }
        break;

    case APP_STATE_CYCLE:
        app_state = APP_STATE_SLEEP;

        if (loramac_compliance_status.Running == true)
        {
            next_tx_time = loramac_app_tx_timeout;
        }
        else
        {
            next_tx_time = loramac_app_tx_timeout + randr(-loramac_app_tx_timeout_rnd, loramac_app_tx_timeout_rnd);
        }

        TimerSetValue(&next_tx_timer, next_tx_time);
        TimerStart(&next_tx_timer);
        break;

    case APP_STATE_SLEEP: {
        CRITICAL_SECTION_BEGIN();
        if (app_can_sleep)
        {
            // Go to sleep here
        }
        else
        {
            // Do some stuff here
            app_can_sleep = true;
        }
        CRITICAL_SECTION_END();
    }
    break;

    default:
        app_state = APP_STATE_START;
        break;
    }

    return app_state;
}

void g_loramac_set_class(DeviceClass_t loramac_class)
{
    app_class = loramac_class;
}

/******************************************************************************
 * g_loramac_get_buffer
 ******************************************************************************/
void *g_loramac_get_buffer(size_t *len)
{
    for (int i = 0; i < MAX_APP_BUFFERS; i++)
    {
        CRITICAL_SECTION_BEGIN();
        if (app_buf[i].available)
        {
            app_buf[i].available = false;
            CRITICAL_SECTION_END();

            if (len)
            {
                *len = MAX_APP_DATA_SIZE;
            }

            return app_buf[i].buf;
        }
        CRITICAL_SECTION_END();
    }

    return NULL;
}

/******************************************************************************
 *  g_loramac_enqueue_uplink
 ******************************************************************************/
void g_loramac_enqueue_uplink(bool ack_mode, uint8_t port, uint8_t *buf, uint8_t len, loramac_app_buf_done_fn done)
{
    loramac_app_data_s *item = ul_data;

    do
    {
        if (item->queued)
        {
            item = &ul_data_queue[item->next];
        }
        else
        {
            item->ack_mode = ack_mode;
            item->port = port;
            item->buf  = buf;
            item->len  = len;
            item->done = done;

            if (port == LORAMAC_COMPLIANCE_TEST_PORT)
            {
                if (loramac_compliance_status.LinkCheck == true)
                {
                    loramac_compliance_status.LinkCheck = false;
                    loramac_compliance_status.State = COMPLIANCE_ACTIVE;
                    
                    item->len = 3;
                    item->buf[0] = 5;
                    item->buf[1] = loramac_compliance_status.DemodMargin;
                    item->buf[2] = loramac_compliance_status.NbGateways;
                }
                else
                {
                    switch (loramac_compliance_status.State)
                    {
                    case COMPLIANCE_RX:
                        loramac_compliance_status.State = COMPLIANCE_ACTIVE;
                        item->len = loramac_compliance_status.AppDataSize;
                        memcpy(item->buf, loramac_compliance_status.AppDataBuffer, item->len);
                        break;
                    case COMPLIANCE_ACTIVE:
                        item->len = 2;
                        item->buf[0] = loramac_compliance_status.DownLinkCounter >> 8;
                        item->buf[1] = loramac_compliance_status.DownLinkCounter;
                        break;
                    }
                }
            }

            if (app_state == APP_STATE_SLEEP)
            {
                app_state = APP_STATE_SEND;
            }
            item->queued = true;
            break;
        }
    } while (item != ul_data);
}
