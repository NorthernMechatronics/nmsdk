/******************************************************************************
 *
 * Filename   : loramac_task.c
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
#include <ctype.h>

/******************************************************************************
 * Ambiq Micro header files
 ******************************************************************************/
#include <am_mcu_apollo.h>
#include <am_bsp.h>
#include <am_util.h>

/******************************************************************************
 * FreeRTOS header files
 ******************************************************************************/
#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>
#include <queue.h>
#include <portmacro.h>
#include <portable.h>
#include <semphr.h>
#include <event_groups.h>

/******************************************************************************
 * LoRaMac-node header files
 ******************************************************************************/
#include <LoRaMac.h>

/******************************************************************************
 * LoRaMac-node adaptation header files
 ******************************************************************************/
#include <rtc-board.h>
#include <sx126x-board.h>

/******************************************************************************
 * Application header files
 ******************************************************************************/
#include "build_timestamp.h"
#include "loramac_app.h"


/******************************************************************************
 * Macros
 ******************************************************************************/
#define LORAMAC_TASK_MQ_SIZE            (6)


/******************************************************************************
 * Type definitions
 ******************************************************************************/
typedef enum
{
  LORAMAC_APP_EVT_NULL,
  LORAMAC_APP_EVT_EXECUTE,
  LORAMAC_APP_EVT_CMD_INIT,
  LORAMAC_APP_EVT_CMD_RESET,
  LORAMAC_APP_EVT_CMD_SEND
}
loramac_app_evt_e;


typedef struct
{
  QueueHandle_t     queue;
  loramac_app_evt_e event;
  uint32_t          data[4];
}
msg_s;


/******************************************************************************
 * Local declarations
 ******************************************************************************/
static QueueHandle_t loramac_task_queue;
static am_hal_rtc_time_t startup_time;
static void (*rtc_alarm_callback)(void) = NULL;
static const char *month_str[] =
{
  "January", "February", "March", "April", "May", "June", "July", "August",
  "September", "October", "November", "December", "Month?"
};
static uint8_t loramac_app_port = DEFAULT_LORAMAC_APP_PORT;
static bool loramac_ack_mode = DEFAULT_LORAMAC_CONFIRMED_MODE; 


/******************************************************************************
 * Forward declarations
 ******************************************************************************/
static portBASE_TYPE s_loramac_cmd(char *out, size_t out_len, const char *cmd);


/******************************************************************************
 * Global declarations
 ******************************************************************************/
const CLI_Command_Definition_t loramac_cmd_def =
{
  (const char * const) "loramac",
  (const char * const) "loramac:\tLoRaMac-node command interface\r\n",
  s_loramac_cmd,
  -1
};

TaskHandle_t loramac_task_handle;


/******************************************************************************
 * Local functions
 ******************************************************************************
 * s_loramac_cmd_help
 * ------------------
 * LoRaMac command interpreter help function
 *
 * Parameters :
 *  @out      - output buffer
 *  @out_len  - output length
 *  @cmd      - command
 *
 ******************************************************************************/
static void s_loramac_cmd_help(char *out, size_t out_len, const char *cmd)
{
  const char *param;
  portBASE_TYPE param_len;

  param = FreeRTOS_CLIGetParameter(cmd, 2, &param_len);

  if(param == NULL)
  {
    strcat(out, "usage: loramac <command>\r\n");
    strcat(out, "\r\n");
    strcat(out, "Supported commands are:\r\n");
    strcat(out, "  init   Initialize the LoRaWAN stack\r\n");
    strcat(out, "  reset  Reset the LoRaWAN stack\r\n");
    strcat(out, "  set    Set stack parameters\r\n");
    strcat(out, "  send   Send an uplink packet\r\n");
  }
  else if(strncmp(param, "init", 4) == 0)
  {
    strcat(out, "usage: loramac init <region>\r\n");
    strcat(out, "  <region> is one of [AS923 | AU915 | EU868 | KR920 | IN865 | US915 | RU864]");
    strcat(out, "  default: US915");
  }
  else if(strncmp(param, "reset", 5) == 0)
  {
    strcat(out, "usage: loramac reset\r\n");
  }
  else if(strncmp(param, "set", 4) == 0)
  {
    strcat(out, "usage: loramac set <option>\r\n");
    strcat(out, "  <option> is one of [ port <num> | ack | noack | class <mode> ]\r\n");
    strcat(out, "  port <num>   Set the port number (range: 0-255)\r\n");
    strcat(out, "  ack      Acknowledge mode on\r\n");
    strcat(out, "  noack    Acknowledge mode off\r\n");
    strcat(out, "  class <value> Set the class (valid values are: A, B, C)\r\n");
    strcat(out, "Note that class change occurs in the next transmission\r\n");
  }
  else if(strncmp(param, "send", 4) == 0)
  {
    strcat(out, "usage: loramac send <string>\r\n");
  }
}

/******************************************************************************
 * s_2ch_to_dec
 * ------------
 * {Insert description here}
 *
 ******************************************************************************/
static uint8_t s_2ch_to_dec(char *in)
{
  int val;

  val = in[1] - '0';
  val += in[0] == ' ' ? 0 : (in[0] - '0') * 10;

  return val;
}

/******************************************************************************
 * s_mth_to_idx
 * ------------
 * {Insert description here}
 *
 ******************************************************************************/
static uint8_t s_mth_to_idx(char *mth)
{
  uint8_t idx;
  
  for(idx = 0; idx < 12; idx++)
  {
    if(am_util_string_strnicmp(mth, month_str[idx], 3) == 0)
    {
      break;
    }
  }

  return idx;
}

/******************************************************************************
 * s_rtc_init
 * ----------
 * Real-Time Clock initialization
 *
 ******************************************************************************/
static void s_rtc_init(void)
{
  char *ts = g_build_timestamp();

  am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);
  am_hal_rtc_osc_select(AM_HAL_RTC_OSC_XT);
  am_hal_rtc_osc_enable();

  startup_time.ui32Hour = s_2ch_to_dec(&ts[12]);
  startup_time.ui32Minute = s_2ch_to_dec(&ts[15]);
  startup_time.ui32Second = s_2ch_to_dec(&ts[18]);
  startup_time.ui32Hundredths = 0;
  startup_time.ui32Month = s_mth_to_idx(&ts[0]);
  startup_time.ui32DayOfMonth = s_2ch_to_dec(&ts[4]);
  startup_time.ui32Year = s_2ch_to_dec(&ts[9]);
  startup_time.ui32Century = s_2ch_to_dec(&ts[7])-20;
  startup_time.ui32Weekday = am_util_time_computeDayofWeek(2000 + startup_time.ui32Century*100 + startup_time.ui32Year, startup_time.ui32Month+1, startup_time.ui32DayOfMonth);
  am_hal_rtc_time_set(&startup_time);

  am_hal_rtc_alarm_interval_set(AM_HAL_RTC_ALM_RPT_100TH);
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
  am_hal_rtc_int_disable(AM_HAL_RTC_INT_ALM);
  NVIC_EnableIRQ(RTC_IRQn);
  NVIC_SetPriority(RTC_IRQn, NVIC_configMAX_SYSCALL_INTERRUPT_PRIORITY);
}

/******************************************************************************
 * s_loramac_execute
 * -----------------
 * Execute the LoRaMac-node state machine
 *
 ******************************************************************************/
static void s_loramac_execute(void)
{
  msg_s msg;

  msg.event = LORAMAC_APP_EVT_EXECUTE;
  xQueueSend(loramac_task_queue, &msg, portMAX_DELAY);
}


/******************************************************************************
 * Global functions
 ******************************************************************************
 * am_rtc_isr
 * ----------
 * Real-Time Clock interrupt service routine
 *
 ******************************************************************************/
void am_rtc_isr(void)
{
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
  if(rtc_alarm_callback)
  {
    rtc_alarm_callback();
  }
}

/******************************************************************************
 * g_loramac_task_set_rtc_alarm_callback
 ******************************************************************************/
void g_loramac_task_set_rtc_alarm_callback(void (*callback)(void))
{
  rtc_alarm_callback = callback;
}

/******************************************************************************
 * s_loramac_cmd
 * -------------
 * LoRaMac command interpreter
 *
 * Parameters  :
 *  @out      - output buffer
 *  @out_len  - output length
 *  @cmd      - command
 *
 * Returns  :
 *  FreeRTOS result
 *
 ******************************************************************************/
static portBASE_TYPE s_loramac_cmd(char *out, size_t out_len, const char *cmd)
{
  const char *param;
  portBASE_TYPE param_len;
  msg_s msg;

  msg.event = LORAMAC_APP_EVT_NULL;
  out[0] = 0x0;
  param = FreeRTOS_CLIGetParameter(cmd, 1, &param_len);
  if(param == NULL)
  {
    s_loramac_cmd_help(out, out_len, cmd);
    return pdFALSE;
  }

  if(strncmp(param, "help", param_len) == 0)
  {
    s_loramac_cmd_help(out, out_len, cmd);
  }
  else if(strncmp(param, "init", param_len) == 0)
  {
    param = FreeRTOS_CLIGetParameter(cmd, 2, &param_len);
    msg.event = LORAMAC_APP_EVT_CMD_INIT;
    if(param == NULL)
    {
      msg.data[0] = LORAMAC_REGION_US915;
    }
    else if(am_util_string_strnicmp(param, "AS923", param_len) == 0)
    {
      msg.data[0] = LORAMAC_REGION_AS923;
    }
    else if(am_util_string_strnicmp(param, "AU915", param_len) == 0)
    {
      msg.data[0] = LORAMAC_REGION_AU915;
    }
    else if(am_util_string_strnicmp(param, "EU868", param_len) == 0)
    {
      msg.data[0] = LORAMAC_REGION_EU868;
    }
    else if(am_util_string_strnicmp(param, "KR920", param_len) == 0)
    {
      msg.data[0] = LORAMAC_REGION_KR920;
    }
    else if(am_util_string_strnicmp(param, "IN865", param_len) == 0)
    {
      msg.data[0] = LORAMAC_REGION_IN865;
    }
    else if(am_util_string_strnicmp(param, "US915", param_len) == 0)
    {
      msg.data[0] = LORAMAC_REGION_US915;
    }
    else if(am_util_string_strnicmp(param, "RU864", param_len) == 0)
    {
      msg.data[0] = LORAMAC_REGION_RU864;
    }
    else
    {
      s_loramac_cmd_help(out, out_len, cmd);
      return pdFALSE;
    }
    strcat(out, "Initializing MAC and joining network\r\n");
  }
  else if(strncmp(param, "reset", param_len) == 0)
  {
    msg.event = LORAMAC_APP_EVT_CMD_RESET;
    strcat(out, "Resetting MAC\r\n");
  }
  else if(strncmp(param, "set", param_len) == 0)
  {
    param = FreeRTOS_CLIGetParameter(cmd, 2, &param_len);
    if(strncmp(param, "ack", param_len) == 0)
    {
      loramac_ack_mode = true;
      strcat(out, "Acknowledge mode on.\r\n");
    }
    else if(strncmp(param, "noack", param_len) == 0)
    {
      loramac_ack_mode = false;
      strcat(out, "Acknowledge mode off.\r\n");
    }
    else if(strncmp(param, "port", param_len) == 0)
    {
      param = FreeRTOS_CLIGetParameter(cmd, 3, &param_len);
      if(param == NULL)
      {
        loramac_app_port = DEFAULT_LORAMAC_APP_PORT;
      }
      else
      {
        loramac_app_port = (uint8_t) atoi(param);
      }
      strcat(out, "Setting port.\r\n");
    }
    else if (strncmp(param, "class", param_len) == 0)
    {
    	param = FreeRTOS_CLIGetParameter(cmd, 3, &param_len);
    	if ((param == NULL) || (param_len > 1))
    	{
    	    strcat(out, "Missing class value\r\n");
    	    return pdFALSE;
    	}
    	else
    	{
    		DeviceClass_t loramac_class;

    		switch(tolower(param[0]))
    		{
    		case 'a':
    			loramac_class = CLASS_A;
    			break;
    		case 'b':
    			loramac_class = CLASS_B;
    			break;
    		case 'c':
    			loramac_class = CLASS_C;
    			break;
    		default:
    			loramac_class = 0xFF;
    			break;
    		}
        	if (loramac_class != 0xFF)
        	{
        		g_loramac_set_class(loramac_class);
        	}
    	}
    }
  }
  else if(strncmp(param, "send", param_len) == 0)
  {
    char *buf;
    size_t len;

    msg.event = LORAMAC_APP_EVT_CMD_SEND;
    buf = g_loramac_get_buffer(&len);

    if(buf == NULL)
    {
      strcat(out, "No free buffers for transmission\r\n");
      return pdFALSE;
    }

    param = FreeRTOS_CLIGetParameter(cmd, 2, &param_len);
    if(param == NULL)
    {
      const char *test_str = "This is a test.";
      len = am_util_string_strlen(test_str);
      am_util_string_strncpy(buf, "This is a test.", len);
    }
    else
    {
      am_util_string_strncpy(buf, param, param_len);
      len = param_len;
    }
    msg.data[0] = (uint32_t) buf;
    msg.data[1] = (uint32_t) ((loramac_ack_mode << 16) | (loramac_app_port << 8) | len);
    msg.data[2] = (uint32_t) NULL;
    strcat(out, "Sending...\r\n");
  }
  if(msg.event != LORAMAC_APP_EVT_NULL)
  {
    xQueueSend(loramac_task_queue, &msg, portMAX_DELAY);
  }

  return pdFALSE;
}

/******************************************************************************
 * g_loramac_isr_execute
 ******************************************************************************/
void g_loramac_isr_execute(void)
{
  msg_s msg;
  portBASE_TYPE preemption = pdFALSE;

  msg.event = LORAMAC_APP_EVT_EXECUTE;
  xQueueSendFromISR(loramac_task_queue, &msg, &preemption);
  portEND_SWITCHING_ISR(preemption);
}

/******************************************************************************
 * g_loramac_task_setup
 ******************************************************************************/
void g_loramac_task_setup(void)
{

	FreeRTOS_CLIRegisterCommand(&loramac_cmd_def);
  
  loramac_task_queue = xQueueCreate(LORAMAC_TASK_MQ_SIZE, sizeof(msg_s));
  s_rtc_init();
}

/******************************************************************************
 * g_loramac_task
 ******************************************************************************/
void g_loramac_task(void *pvp)
{
  // LoRaMac-node adapter initialization
  RtcInit();
  SX126xIoInit();

  while(1)
  {
    msg_s msg;

    g_loramac_state_machine();
    if(xQueueReceive(loramac_task_queue, &msg, 500) == pdPASS)
    {
      switch(msg.event)
      {
        case LORAMAC_APP_EVT_EXECUTE:
          g_loramac_state_machine();
          s_loramac_execute();
          break;

        case LORAMAC_APP_EVT_CMD_INIT:
          g_loramac_init((LoRaMacRegion_t) msg.data[0]);
          s_loramac_execute();
          break;

        case LORAMAC_APP_EVT_CMD_RESET:
          g_loramac_reset();
          break;

        case LORAMAC_APP_EVT_CMD_SEND:
          g_loramac_enqueue_uplink((bool) ((msg.data[1] & 0xFF0000) != 0), (uint8_t) ((msg.data[1] & 0xFF00) >> 8), (uint8_t *) msg.data[0], (uint8_t) (msg.data[1] & 0xFF), (loramac_app_buf_done_fn) msg.data[2]);
          break;

        default:
          am_util_debug_printf("Other message (0x%08X) received.\n", msg.event);
          break;
      }
    }
  }
}
