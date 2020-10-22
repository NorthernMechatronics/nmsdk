/******************************************************************************
 *
 * Filename     : console_task.c
 * Description  : Console task implementation
 *
 ******************************************************************************/

/******************************************************************************
 * Standard header files
 ******************************************************************************/
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <am_bsp.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <queue.h>
#include <semphr.h>
#include <task.h>

#include "console_task.h"
#include "build_timestamp.h"

/******************************************************************************
 * Macros
 ******************************************************************************/
#define MAX_CMD_HIST_LEN (8)
#define MAX_INPUT_LEN (128)

/******************************************************************************
 * Local declarations
 ******************************************************************************/
static SemaphoreHandle_t g_sConsoleMutex;

static char in_str[MAX_INPUT_LEN];
static uint8_t in_len = 0;

static const char *cmd_prompt = "> ";
static const char *welcome_msg =
    "\r\n"
    "Northern Mechatronics\r\n\r\n"
    "NM180100 Command Console\r\n";

static char cmd_hist[MAX_CMD_HIST_LEN][MAX_INPUT_LEN];
static uint8_t cmd_hist_len = 0;
static uint8_t cmd_hist_first = 0;
static uint8_t cmd_hist_last = 0;
static uint8_t cmd_hist_cur = 0;

static const char *crlf = "\r\n";

#define CONSOLE_UART_BUFFER_SIZE 128
static QueueHandle_t g_sConsoleQueueHandle;

static void *g_sCOMUART;

static am_hal_uart_config_t g_sConsoleUartBufferedConfig = {
    //
    // Standard UART settings: 115200-8-N-1
    //
    .ui32BaudRate = 115200,
    .ui32DataBits = AM_HAL_UART_DATA_BITS_8,
    .ui32Parity = AM_HAL_UART_PARITY_NONE,
    .ui32StopBits = AM_HAL_UART_ONE_STOP_BIT,
    .ui32FlowControl = AM_HAL_UART_FLOW_CTRL_NONE,

    //
    // Set TX and RX FIFOs to interrupt at half-full.
    //
    .ui32FifoLevels = (AM_HAL_UART_TX_FIFO_3_4 | AM_HAL_UART_RX_FIFO_3_4),

    //
    // The default interface will just use polling instead of buffers.
    //
    .pui8TxBuffer = 0,
    .ui32TxBufferSize = 0,
    .pui8RxBuffer = 0,
    .ui32RxBufferSize = 0,
};

/******************************************************************************
 * Global declarations
 ******************************************************************************/
TaskHandle_t console_task_handle;

/******************************************************************************
 * Local functions
 ******************************************************************************
 * s_cmd_hist_add
 * --------------
 * Adds a command to the end of the command history buffer
 *
 * Parameters :
 *  @cmd      - {description}
 *
 ******************************************************************************/
static void s_cmd_hist_add(const char *cmd, size_t len) {
  uint8_t next;

  if (cmd_hist_len < MAX_CMD_HIST_LEN) {
    next = cmd_hist_len;
    cmd_hist_len++;
  } else {
    next = (cmd_hist_last + 1) % MAX_CMD_HIST_LEN;
  }
  strncpy(cmd_hist[next], cmd, len);
  cmd_hist_last = next;
  cmd_hist_cur = next;
  // wrap-around
  if (cmd_hist_last == cmd_hist_first && cmd_hist_len == MAX_CMD_HIST_LEN) {
    cmd_hist_first = (cmd_hist_first + 1) % MAX_CMD_HIST_LEN;
  }
}

/******************************************************************************
 * s_cmd_hist_up
 * --------------
 * Moves up the command history buffer
 *
 * Returns  :
 *  Command from history buffer
 *
 ******************************************************************************/
static const char *s_cmd_hist_up(void) {
  const char *cmd;

  // Empty history buffer
  if (cmd_hist_len == 0) {
    return NULL;
  }

  if (cmd_hist_cur == cmd_hist_len) {
    cmd = NULL;
  } else {
    cmd = cmd_hist[cmd_hist_cur];
  }
  cmd_hist_cur = (cmd_hist_cur + cmd_hist_len) % (cmd_hist_len + 1);

  return cmd;
}

/******************************************************************************
 * s_cmd_hist_down
 * --------------
 * Moves down the command history buffer
 *
 * Returns  :
 *  Command from history buffer
 *
 ******************************************************************************/
static const char *s_cmd_hist_down(void) {
  // Empty history buffer
  if (cmd_hist_len == 0) {
    return NULL;
  }

  cmd_hist_cur = (cmd_hist_cur + 1) % (cmd_hist_len + 1);
  if (cmd_hist_cur == cmd_hist_len) {
    return NULL;
  }

  return cmd_hist[cmd_hist_cur];
}

/******************************************************************************
 * s_clearln
 * ---------
 * Erases the current line on the screen
 *
 * Parameters :
 *    @pos    - Position to delete from
 *
 ******************************************************************************/
static void s_clearln(uint8_t pos) {
  char buf[16];

  if (pos > 0) {
    // ANSI escape code CUB
    am_util_stdio_sprintf(buf, "\e[%dD\e[0K", pos);
    g_console_print(buf);
  }
}

/******************************************************************************
 * Global functions
 ******************************************************************************
 * g_console_print
 * ---------------
 * Print a null-terminated string to the console
 *
 * Parameters :
 *  @str      - Null-terminated string
 *
 ******************************************************************************/
void g_console_print(const char *str) { g_console_write(str, strlen(str)); }

void g_console_prompt()
{
  g_console_print(cmd_prompt);
}

/******************************************************************************
 * g_console_write
 * ---------------
 * Write a string of specified length to the console
 *
 * Parameters :
 *  @buf      - Buffer to output
 *  @len      - Number of characters from buffer to output
 *
 ******************************************************************************/
void g_console_write(const char *buf, size_t len) {
  xSemaphoreTake(g_sConsoleMutex, portMAX_DELAY);

  uint8_t *pui8String = (uint8_t *)buf;
  uint32_t ui32BytesWritten = 0;
  uint32_t ui32TotalLength = len;
  uint32_t ui32TotalWritten = 0;

  am_hal_uart_transfer_t sUartWrite = {
      .ui32Direction = AM_HAL_UART_WRITE,
      .pui8Data = pui8String,
      .ui32NumBytes = ui32TotalLength < CONSOLE_UART_BUFFER_SIZE
                          ? ui32TotalLength
                          : CONSOLE_UART_BUFFER_SIZE,
      .ui32TimeoutMs = 0,
      .pui32BytesTransferred = &ui32BytesWritten,
  };

  while (ui32TotalWritten < ui32TotalLength) {
    am_hal_uart_transfer(g_sCOMUART, &sUartWrite);
    pui8String += ui32BytesWritten;
    len -= ui32BytesWritten;
    ui32TotalWritten += ui32BytesWritten;

    sUartWrite.pui8Data = pui8String;
    sUartWrite.ui32NumBytes = ui32TotalLength < CONSOLE_UART_BUFFER_SIZE
                                  ? ui32TotalLength
                                  : CONSOLE_UART_BUFFER_SIZE;
  }

  xSemaphoreGive(g_sConsoleMutex);
}

char g_console_read_char() {
  size_t received = 0;
  uint8_t ch;

  while (received < 1) {
    if (xQueueReceive(g_sConsoleQueueHandle, &ch, portMAX_DELAY) == pdPASS) {
      received++;
    }
  }

  return ch;
}

/******************************************************************************
 * g_console_task_setup
 * --------------------
 * Console task setup function
 *
 ******************************************************************************/
void g_console_task_setup(void) {
  g_sConsoleMutex = xSemaphoreCreateMutex();
  g_sConsoleQueueHandle =
      xQueueCreate(CONSOLE_UART_BUFFER_SIZE, sizeof(uint8_t));
  //
  // Initialize, power up, and configure the communication UART. Use the
  // custom configuration if it was provided. Otherwise, just use the default
  // configuration.
  //
  am_hal_uart_initialize(CONSOLE_UART_INST, &g_sCOMUART);
  am_hal_uart_power_control(g_sCOMUART, AM_HAL_SYSCTRL_WAKE, false);
  am_hal_uart_configure(g_sCOMUART, &g_sConsoleUartBufferedConfig);

  //
  // Enable the UART pins.
  //
  am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_TX, g_AM_BSP_GPIO_COM_UART_TX);
  am_hal_gpio_pinconfig(AM_BSP_GPIO_COM_UART_RX, g_AM_BSP_GPIO_COM_UART_RX);

  //
  // Enable the interrupts for the UART.
  //
  am_hal_uart_interrupt_enable(g_sCOMUART, AM_HAL_UART_INT_TXCMP |
                                               AM_HAL_UART_INT_RX_TMOUT |
                                               AM_HAL_UART_INT_RX);
  NVIC_SetPriority((IRQn_Type)(UART0_IRQn + CONSOLE_UART_INST),
                   NVIC_configMAX_SYSCALL_INTERRUPT_PRIORITY);
  NVIC_EnableIRQ((IRQn_Type)(UART0_IRQn + CONSOLE_UART_INST));

  memset(cmd_hist, 0, MAX_CMD_HIST_LEN * MAX_INPUT_LEN);
}

/******************************************************************************
 * g_console_task
 * --------------
 * Console task entrypoint function
 *
 * Parameters :
 *  @pvp      - FreeRTOS parameters
 *
 ******************************************************************************/
void g_console_task(void *pvp) {
  char ch;
  char *out_str;
  portBASE_TYPE ret;

  out_str = FreeRTOS_CLIGetOutputBuffer();

  g_console_task_setup();

  g_console_print(welcome_msg);
  g_console_print("Built on: ");
  g_console_print(g_build_timestamp());
  g_console_print(crlf);
  g_console_print(crlf);
  g_console_print(cmd_prompt);

  while (1) {
    ch = g_console_read_char();

    switch ((uint8_t)ch) {
      case '\e':
        ch = g_console_read_char();
        ch = g_console_read_char();
        if (ch == 'A') {
          const char *cmd = s_cmd_hist_up();

          s_clearln(in_len);
          if (cmd != NULL) {
            strcpy(in_str, cmd);
            in_len = strlen(in_str);
            g_console_write(in_str, in_len);
          } else {
            in_len = 0;
          }
        } else if (ch == 'B') {
          const char *cmd = s_cmd_hist_down();

          s_clearln(in_len);
          if (cmd != NULL) {
            strcpy(in_str, cmd);
            in_len = strlen(in_str);
            g_console_write(in_str, in_len);
          } else {
            in_len = 0;
          }
        }
        break;

      case '\b':
      case '\x7f':
        if (in_len > 0) {
          s_clearln(in_len--);

          in_str[in_len] = '\0';
          g_console_write(in_str, in_len);
        }
        break;

      case '\r':
      case '\n':
        g_console_print(crlf);

        if (in_len == 0) {
          g_console_print(cmd_prompt);
          cmd_hist_cur = cmd_hist_last;
          break;
        }

        do {
          ret = FreeRTOS_CLIProcessCommand(in_str, out_str,
                                           configCOMMAND_INT_MAX_OUTPUT_SIZE);
          g_console_print(out_str);
        } while (ret != pdFALSE);

        g_console_print(crlf);
        g_console_print(cmd_prompt);

        s_cmd_hist_add(in_str, in_len);
        in_len = 0;
        memset(in_str, 0x00, MAX_INPUT_LEN);
        break;

      default:
        g_console_write(&ch, sizeof(ch));

        if ((ch >= ' ') && (ch <= '~')) {
          if (in_len < MAX_INPUT_LEN) {
            in_str[in_len] = ch;
            in_len++;
          }
        }
        break;
    }
  }
}

void am_uart_isr() {
  portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

  uint32_t ui32Status, ui32Idle;
  am_hal_uart_interrupt_status_get(g_sCOMUART, &ui32Status, true);
  am_hal_uart_interrupt_clear(g_sCOMUART, ui32Status);
  am_hal_uart_interrupt_service(g_sCOMUART, ui32Status, &ui32Idle);

  if ((ui32Status & AM_HAL_UART_INT_RX) ||
      (ui32Status & AM_HAL_UART_INT_RX_TMOUT)) {
    uint8_t ui8Buffer[32];
    uint32_t ui32BytesRead = 0;
    am_hal_uart_transfer_t sUartRead = {
        .ui32Direction = AM_HAL_UART_READ,
        .pui8Data = ui8Buffer,
        .ui32NumBytes = 32,
        .ui32TimeoutMs = 0,
        .pui32BytesTransferred = &ui32BytesRead,
    };

    am_hal_uart_transfer(g_sCOMUART, &sUartRead);

    for (int i = 0; i < ui32BytesRead; i++) {
      xQueueSendFromISR(g_sConsoleQueueHandle, &ui8Buffer[i],
                        &xHigherPriorityTaskWoken);
    }
  }

  portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
