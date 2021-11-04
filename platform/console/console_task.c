/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, Northern Mechatronics, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include <am_bsp.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <stream_buffer.h>
#include <task.h>

#include "build_timestamp.h"
#include "console_task.h"

/******************************************************************************
 * Macros
 ******************************************************************************/
#define MAX_CMD_HIST_LEN (8)
#define MAX_INPUT_LEN    (128)

#define STREAM_BUFFER_SIZE  64

/******************************************************************************
 * Local declarations
 ******************************************************************************/
static volatile StreamBufferHandle_t stream_buffer;

static char    in_str[MAX_INPUT_LEN];
static uint8_t in_len = 0;

static const char * const cmd_prompt  = "> ";
static const char * const welcome_msg = "\r\n"
                                 "Northern Mechatronics\r\n\r\n"
                                 "NM180100 Command Console\r\n";

static char    cmd_hist[MAX_CMD_HIST_LEN][MAX_INPUT_LEN];
static uint8_t cmd_hist_len   = 0;
static uint8_t cmd_hist_first = 0;
static uint8_t cmd_hist_last  = 0;
static uint8_t cmd_hist_cur   = 0;

static const char * const crlf = "\r\n";

/******************************************************************************
 * Global declarations
 ******************************************************************************/
TaskHandle_t nm_console_task_handle;

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
static void s_cmd_hist_add(const char *cmd, size_t len)
{
    uint8_t next;

    if (cmd_hist_len < MAX_CMD_HIST_LEN) {
        next = cmd_hist_len;
        cmd_hist_len++;
    } else {
        next = (cmd_hist_last + 1) % MAX_CMD_HIST_LEN;
    }
    strncpy(cmd_hist[next], cmd, len);
    cmd_hist[next][len] = '\x0';
    cmd_hist_last = next;
    cmd_hist_cur  = next;
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
static const char *s_cmd_hist_up(void)
{
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
static const char *s_cmd_hist_down(void)
{
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
static void s_clearln(uint8_t pos)
{
    if (pos > 0) {
        // ANSI escape code CUB
        am_util_stdio_printf("\e[%dD\e[0K", pos);
    }
}

void nm_console_print_prompt() { am_util_stdio_printf(cmd_prompt); }

char nm_console_read()
{
    uint8_t ch;

    xStreamBufferReceive(stream_buffer, &ch, 1, portMAX_DELAY );

    return ch;
}

/******************************************************************************
 * g_console_task_setup
 * --------------------
 * Console task setup function
 *
 ******************************************************************************/
void g_console_task_setup(void)
{
    am_bsp_buffered_uart_printf_enable();
    NVIC_SetPriority((IRQn_Type)(UART0_IRQn + AM_BSP_UART_PRINT_INST), 4);

    memset(cmd_hist, 0, MAX_CMD_HIST_LEN * MAX_INPUT_LEN);

    stream_buffer = xStreamBufferCreate (
            STREAM_BUFFER_SIZE,
            1);
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
void nm_console_task(void *pvp)
{
    char          ch;
    char *        out_str;
    portBASE_TYPE ret;

    out_str = FreeRTOS_CLIGetOutputBuffer();

    g_console_task_setup();

    am_util_stdio_printf(welcome_msg);
    am_util_stdio_printf("Built on: ");
    am_util_stdio_printf(build_timestamp());
    am_util_stdio_printf(crlf);
    am_util_stdio_printf(crlf);
    am_util_stdio_printf(cmd_prompt);

    while (1) {
        ch = nm_console_read();

        switch ((uint8_t)ch) {
        case '\e':
            ch = nm_console_read();
            ch = nm_console_read();
            if (ch == 'A') {
                const char *cmd = s_cmd_hist_up();

                s_clearln(in_len);
                if (cmd != NULL) {
                    strcpy(in_str, cmd);
                    am_util_stdio_printf(in_str);
                    in_len = strlen(in_str);
                } else {
                    in_len = 0;
                }
            } else if (ch == 'B') {
                const char *cmd = s_cmd_hist_down();

                s_clearln(in_len);
                if (cmd != NULL) {
                    strcpy(in_str, cmd);
                    am_util_stdio_printf(in_str);
                    in_len = strlen(in_str);
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
                am_util_stdio_printf(in_str);
            }
            break;

        case '\r':
        case '\n':
            //am_util_stdio_printf(crlf);
            am_bsp_uart_string_print(crlf);
            if (in_len == 0) {
                //am_util_stdio_printf(cmd_prompt);
                am_bsp_uart_string_print(cmd_prompt);
                cmd_hist_cur = cmd_hist_last;
                break;
            }

            do {
                ret = FreeRTOS_CLIProcessCommand(
                    in_str, out_str, configCOMMAND_INT_MAX_OUTPUT_SIZE);
                am_util_stdio_printf(out_str);
            } while (ret != pdFALSE);

            am_util_stdio_printf(crlf);
            am_util_stdio_printf(cmd_prompt);

            s_cmd_hist_add(in_str, in_len);
            in_len = 0;
            memset(in_str, 0x00, MAX_INPUT_LEN);
            break;

        default:
            am_util_stdio_printf("%c", ch);

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

void am_uart_isr()
{
    am_bsp_buffered_uart_service();

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t received = 0;
    uint8_t  buffer[32];

    am_hal_uart_transfer_t uart_transfer = {
        .ui32Direction         = AM_HAL_UART_READ,
        .pui8Data              = buffer,
        .ui32NumBytes          = 32,
        .ui32TimeoutMs         = 0,
        .pui32BytesTransferred = &received,
    };
    am_bsp_com_uart_transfer(&uart_transfer);

    if (received > 0)
    {
        xStreamBufferSendFromISR(stream_buffer, (void*)buffer, received, &xHigherPriorityTaskWoken);
    }

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}
