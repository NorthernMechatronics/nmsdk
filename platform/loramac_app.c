/******************************************************************************
 *
 * Filename   : loramac_app.c
 * Description  : LoRaMac task implementation
 *
 ******************************************************************************/


/******************************************************************************
 * Standard header files
 ******************************************************************************/
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

/******************************************************************************
 * Ambiq Micro header files
 ******************************************************************************/
#include <am_mcu_apollo.h>
#include <am_bsp.h>
#include <am_util.h>

/******************************************************************************
 * LoRaMac-node header files
 ******************************************************************************/
#include <LoRaMac.h>
#include <LoRaMacTest.h>

/******************************************************************************
 * LoRaMac-node adaptation header files
 ******************************************************************************/
#include <utilities.h>
#include <board.h>
#include <gpio.h>

/******************************************************************************
 * Application header files
 ******************************************************************************/
#include "loramac_app.h"


/******************************************************************************
 * Macros
 ******************************************************************************/
#define APP_TX_NEXT_TIMEOUT             (5000)
#define APP_TX_NEXT_TIMEOUT_RND         (1000)
#define MAX_APP_TX_QUEUE_SIZE           (3)
#define MAX_APP_BUFFERS                 (3)


/******************************************************************************
 * Type definitions
 ******************************************************************************/
typedef struct
{
  bool                        queued;
  bool                        sent;
  uint8_t                     *buf;
  uint8_t                     len;
  loramac_app_buf_done_fn     done;

  bool                        ack_mode;
  uint8_t                     port;

  uint8_t                     next;
}
loramac_app_data_s;

typedef struct
{
  bool                        available;
  uint8_t                     buf[MAX_APP_DATA_SIZE];
}
loramac_app_buffer_s;


/******************************************************************************
 * Local declarations
 ******************************************************************************/
static loramac_app_state_e app_state = APP_STATE_NULL;

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

static const char *loramac_req_stat_str[] =
{
  "OK",
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
  "ERROR"
};

static const char* loramac_mlstat_str[] =
{
  "OK",
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
  "BEACON_NOT_FOUND"
};


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

  for(uint8_t i = 0; i < len; i++)
  {
    if((i % width) == 0 && prefix)
    {
      am_util_debug_printf(prefix);
    }
    if(((i+1) % width) == 0)
    {
      am_util_debug_printf("%02X\n", buf[i]);
    }
    else
    {
      am_util_debug_printf("%02X%c", buf[i], sp);
    }
  }
  if(len % width != 0)
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

  if(status == LORAMAC_STATUS_OK)
  {
    am_util_debug_printf("###### ===== JOINING ==== ######\n");
    app_state = APP_STATE_IDLE;
  }
  else
  {
    if(status == LORAMAC_STATUS_DUTYCYCLE_RESTRICTED)
    {
      am_util_debug_printf("Next Tx in  : %lu [ms]\n", mlme_req.ReqReturn.DutyCycleWaitTime);
    }
    app_state = APP_STATE_SCHEDULE;
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

  if(LoRaMacQueryTxPossible(ul_data->len, &tx_info) != LORAMAC_STATUS_OK)
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
    if(ul_data->ack_mode)
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

  if(status == LORAMAC_STATUS_OK)
  {
    return reschedule;
  }

  if(status == LORAMAC_STATUS_DUTYCYCLE_RESTRICTED)
  {
    am_util_debug_printf("Next Tx in  : %lu [ms]\n", mcps_req.ReqReturn.DutyCycleWaitTime);
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

  if(status == LORAMAC_STATUS_OK)
  {
    if(mib_req.Param.NetworkActivation == ACTIVATION_TYPE_NONE)
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
  if(mcps_cnf->Status == LORAMAC_EVENT_INFO_STATUS_OK)
  {
    switch(mcps_cnf->McpsRequest)
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

  if(ul_data->len != 0 && ul_data->sent)
  {
    am_util_debug_printf("TX DATA     : ");
    if(ul_data->ack_mode)
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

  mib_get.Type  = MIB_CHANNELS;
  if(LoRaMacMibGetRequestConfirm(&mib_get)== LORAMAC_STATUS_OK)
  {
    am_util_debug_printf("U/L FREQ    : %lu\n", mib_get.Param.ChannelList[mcps_cnf->Channel].Frequency);
  }

  am_util_debug_printf("TX POWER    : %d\n", mcps_cnf->TxPower);

  mib_get.Type  = MIB_CHANNELS_MASK;
  if(LoRaMacMibGetRequestConfirm(&mib_get)== LORAMAC_STATUS_OK)
  {
    am_util_debug_printf("CHANNEL MASK: ");
    for(uint8_t i = 0; i < loramac_channel_mask_count; i++)
    {
      am_util_debug_printf("%04X ", mib_get.Param.ChannelsMask[i]);
    }
  }
  am_util_debug_printf("\n");
  if(ul_data->queued && ul_data->sent)
  {
    if(ul_data->done)
    {
      ul_data->done(ul_data->buf, true);
    }
    else
    {
      for(int i = 0; i < MAX_APP_BUFFERS; i++)
      {
        if(ul_data->buf >= app_buf[i].buf && ul_data->buf < app_buf[i].buf+MAX_APP_DATA_SIZE)
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
  if(mcps_ind->Status != LORAMAC_EVENT_INFO_STATUS_OK)
  {
    return;
  }

  switch(mcps_ind->McpsIndication)
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

  if(mcps_ind->FramePending == true)
  {
    next_tx_expiry_fn(NULL);
  }

  if(mcps_ind->RxData == true)
  {
    // Do something with the data
  }

  const char *slotStrings[] = {"1", "2", "C", "C Multicast", "B Ping-Slot", "B Multicast Ping-Slot" };

  am_util_debug_printf("\n###### ===== DOWNLINK FRAME %lu ==== ######\n", mcps_ind->DownLinkCounter);
  am_util_debug_printf("RX WINDOW   : %s\n", slotStrings[mcps_ind->RxSlot]);
  am_util_debug_printf("RX PORT     : %d\n", mcps_ind->Port);
  if(mcps_ind->BufferSize != 0)
  {
    am_util_debug_printf("RX DATA   : ");
    if(mcps_ind->McpsIndication == MCPS_UNCONFIRMED)
    {
      am_util_debug_printf("UNCONFIRMED\n");
    }
    else if(mcps_ind->McpsIndication == MCPS_CONFIRMED)
    {
      am_util_debug_printf("CONFIRMED\n");
    }
    else if(mcps_ind->McpsIndication == MCPS_MULTICAST)
    {
      am_util_debug_printf("MULTICAST\n");
    }
    else if(mcps_ind->McpsIndication == MCPS_PROPRIETARY)
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
  switch(mlme_cnf->MlmeRequest)
  {
    case MLME_JOIN:
      if(mlme_cnf->Status == LORAMAC_EVENT_INFO_STATUS_OK)
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
        app_state = APP_STATE_SEND;
      }
      else
      {
        s_join_network();
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
  if(mlme_ind->Status != LORAMAC_EVENT_INFO_STATUS_BEACON_LOCKED)
  {
    am_util_debug_printf("\n###### ===== MLME-Indication ==== ######\n");
    am_util_debug_printf("STATUS      : %s\n", loramac_mlstat_str[mlme_ind->Status]);
  }
  switch(mlme_ind->MlmeIndication)
  {
    case MLME_SCHEDULE_UPLINK:
      next_tx_expiry_fn(NULL);
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

  switch(region)
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
  if(status != LORAMAC_STATUS_OK)
  {
    am_util_debug_printf("LoRaMac wasn't properly initialized, error: %s", loramac_req_stat_str[status]);
    return false;
  }

  for(int i = 0; i < MAX_APP_TX_QUEUE_SIZE; i++)
  {
    ul_data_queue[i].queued = false;
    ul_data_queue[i].buf = NULL;
    ul_data_queue[i].len = 0;
    ul_data_queue[i].done = NULL;

    ul_data_queue[i].ack_mode = false;
    ul_data_queue[i].port = 0;

    ul_data_queue[i].next = (i + 1) % MAX_APP_TX_QUEUE_SIZE;
  }
  for(int i = 0; i < MAX_APP_BUFFERS; i++)
  {
    app_buf[i].available = true;
  }
  app_state = APP_STATE_INIT;
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

  if(app_state == APP_STATE_NULL)
  {
    return app_state;
  }
  if(Radio.IrqProcess != NULL)
  {
    Radio.IrqProcess();
  }
  LoRaMacProcess();
  switch(app_state)
  {
    case APP_STATE_INIT:
      TimerInit(&next_tx_timer, next_tx_expiry_fn);

      mib_req.Type = MIB_PUBLIC_NETWORK;
      mib_req.Param.EnablePublicNetwork = true;
      LoRaMacMibSetRequestConfirm(&mib_req);

      mib_req.Type = MIB_ADR;
      mib_req.Param.AdrEnable = DEFAULT_LORAMAC_ADR_MODE;
      LoRaMacMibSetRequestConfirm(&mib_req);

      if(loramac_duty_cycle_used)
      {
        LoRaMacTestSetDutyCycleOn(true);
      }

      mib_req.Type = MIB_SYSTEM_MAX_RX_ERROR;
      mib_req.Param.SystemMaxRxError = 20;
      LoRaMacMibSetRequestConfirm(&mib_req);

      LoRaMacStart();

      mib_req.Type = MIB_NETWORK_ACTIVATION;
      status = LoRaMacMibGetRequestConfirm(&mib_req);

      if(status == LORAMAC_STATUS_OK)
      {
        if(mib_req.Param.NetworkActivation == ACTIVATION_TYPE_NONE)
        {
          app_state = APP_STATE_JOIN;
        }
        else if(ul_data->queued)
        {
          app_state = APP_STATE_SEND;
        }
        else
        {
          app_state = APP_STATE_IDLE;
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

      s_join_network();
      break;

    case APP_STATE_SEND:
      if(ul_data->queued == true)
      {
        if(s_loramac_send_frame())
        {
          app_state = APP_STATE_SCHEDULE;
        }
        else
        {
          app_state = APP_STATE_IDLE;
        }
      }
      else
      {
        app_state = APP_STATE_IDLE;
      }
      break;

    case APP_STATE_SCHEDULE:
      app_state = APP_STATE_IDLE;
      next_tx_time = APP_TX_NEXT_TIMEOUT + randr(-APP_TX_NEXT_TIMEOUT_RND, APP_TX_NEXT_TIMEOUT_RND);
      TimerSetValue(&next_tx_timer, next_tx_time);
      TimerStart(&next_tx_timer);
      break;

    case APP_STATE_IDLE:
      {
        CRITICAL_SECTION_BEGIN();
        if(app_can_sleep)
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
      app_state = APP_STATE_INIT;
      break;
  }

  return app_state;
}

/******************************************************************************
 * g_loramac_get_buffer
 ******************************************************************************/
void *g_loramac_get_buffer(size_t *len)
{
  for(int i = 0; i < MAX_APP_BUFFERS; i++)
  {
    CRITICAL_SECTION_BEGIN();
    if(app_buf[i].available)
    {
      app_buf[i].available = false;
      CRITICAL_SECTION_END();

      if(len)
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
    if(item->queued)
    {
      item = &ul_data_queue[item->next];
    }
    else
    {
      item->ack_mode = ack_mode;
      item->port = port;
      item->buf = buf;
      item->len = len;
      item->done = done;

      if(app_state == APP_STATE_IDLE)
      {
        app_state = APP_STATE_SEND;
      }
      item->queued = true;
      break;
    }
  }
  while(item != ul_data);
}
