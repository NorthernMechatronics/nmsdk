/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2020, Northern Mechatronics, Inc.
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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <am_bsp.h>
#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>

#include "iom_service.h"

TaskHandle_t nm_iom_task_handle;

#define BUFFER_MAX_SIZE 64
#define OPCODE_MAX_SIZE 8

static void *g_sIomHandler[AM_REG_IOM_NUM_MODULES];
static am_hal_iom_config_t g_sIomConfig[AM_REG_IOM_NUM_MODULES];
static const uint8_t gui8IomReserved[] = {3, 4};
static size_t MAX_RESERVED_IOM = sizeof(gui8IomReserved);

portBASE_TYPE prvIomCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                            const char *pcCommandString);

const CLI_Command_Definition_t prvIomCommandDefinition = {
    (const char *const) "iom",
    (const char *const) "iom:\tApollo IO module configuration and control.\r\n",
    prvIomCommand, -1};

static bool IomIsReserved(uint32_t port)
{
    for (int i = 0; i < MAX_RESERVED_IOM; i++) {
        if (gui8IomReserved[i] == port)
            return true;
    }

    return false;
}

static void IomHelp(char *pcWriteBuffer)
{
    strcat(pcWriteBuffer, "usage: iom <command> [<args>]\r\n");
    strcat(pcWriteBuffer, "\r\n");
    strcat(pcWriteBuffer, "Supported commands are:\r\n");
    strcat(pcWriteBuffer, "  open\tIOM port\r\n");
    strcat(pcWriteBuffer, "  write\twrite data to IOM port\r\n");
    strcat(pcWriteBuffer, "  read\tread data from IOM port\r\n");
    strcat(pcWriteBuffer, "  help\tshow command details\r\n");
    strcat(pcWriteBuffer, "\r\n");
    strcat(pcWriteBuffer, "See 'iom help <command>' for command details.\r\n");
}

static void IomFormatData(const char *in, size_t inlen, uint8_t *out,
                          size_t *outlen)
{
    size_t n = 0;
    char cNum[3];
    *outlen = 0;
    while (n < inlen) {
        switch (in[n]) {
        case '\\':
            n++;
            switch (in[n]) {
            case 'x':
                n++;
                memset(cNum, 0, 3);
                memcpy(cNum, &in[n], 2);
                n++;
                out[*outlen] = strtol(cNum, NULL, 16);
                break;
            }
            break;
        default:
            out[*outlen] = in[n];
            break;
        }
        *outlen = *outlen + 1;
        n++;
    }
}

static void IomHelpSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                              const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    if (pcParameterString == NULL) {
        IomHelp(pcWriteBuffer);
        return;
    }

    if (strncmp(pcParameterString, "open", xParameterStringLength) == 0) {
        strcat(pcWriteBuffer,
               "usage: iom open [port number] [mode] <freq> <clock>\r\n");
        strcat(pcWriteBuffer, "  mode  is either SPI or I2C\r\n");
        strcat(pcWriteBuffer,
               "  freq  is in Hz and must be one of the following:\r\n");
        strcat(pcWriteBuffer, "      48000000\r\n");
        strcat(pcWriteBuffer, "      24000000\r\n");
        strcat(pcWriteBuffer, "      16000000\r\n");
        strcat(pcWriteBuffer, "      12000000\r\n");
        strcat(pcWriteBuffer, "       8000000\r\n");
        strcat(pcWriteBuffer, "       6000000\r\n");
        strcat(pcWriteBuffer, "       4000000 (default)\r\n");
        strcat(pcWriteBuffer, "       3000000\r\n");
        strcat(pcWriteBuffer, "       2000000\r\n");
        strcat(pcWriteBuffer, "       1500000\r\n");
        strcat(pcWriteBuffer, "       1000000\r\n");
        strcat(pcWriteBuffer, "        750000\r\n");
        strcat(pcWriteBuffer, "        500000\r\n");
        strcat(pcWriteBuffer, "        400000\r\n");
        strcat(pcWriteBuffer, "        375000\r\n");
        strcat(pcWriteBuffer, "        250000\r\n");
        strcat(pcWriteBuffer, "        125000\r\n");
        strcat(pcWriteBuffer, "        100000\r\n");
        strcat(pcWriteBuffer, "         50000\r\n");
        strcat(pcWriteBuffer, "         10000\r\n");
        strcat(pcWriteBuffer,
               "  clock  is one of the following (SPI mode only):\r\n");
        strcat(pcWriteBuffer, "      0  CPOL = 0, CPHA = 0 (default)\r\n");
        strcat(pcWriteBuffer, "      1  CPOL = 1, CPHA = 0\r\n");
        strcat(pcWriteBuffer, "      2  CPOL = 0, CPHA = 1\r\n");
        strcat(pcWriteBuffer, "      3  CPOL = 1, CPHA = 1\r\n");
    } else if (strncmp(pcParameterString, "close", xParameterStringLength) ==
               0) {
        strcat(pcWriteBuffer, "usage: iom close [port number]\r\n");
    } else if (strncmp(pcParameterString, "write", xParameterStringLength) ==
               0) {
        strcat(pcWriteBuffer,
               "usage: iom write [port number] [data length] [data]\r\n");
    } else if (strncmp(pcParameterString, "read", xParameterStringLength) ==
               0) {
        strcat(pcWriteBuffer,
               "usage: iom read [port number] [bytes to read] [opcode]\r\n");
    }
}

static void IomOpenSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                              const char *pcCommandString)
{
    const char *pcParameterString;
    const char *pcPortString;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    int port = -1;
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing port number\r\n");
        return;
    } else {
        pcPortString = pcParameterString;
        port = atoi(pcParameterString);
    }

    if (IomIsReserved(port)) {
        strcat(pcWriteBuffer, "error: requested port is reserved\r\n");
        return;
    }

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 3, &xParameterStringLength);
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing port mode\r\n");
        return;
    } else {
        if (strncmp(pcParameterString, "SPI", xParameterStringLength) == 0) {
            g_sIomConfig[port].eInterfaceMode = AM_HAL_IOM_SPI_MODE;
        } else if (strncmp(pcParameterString, "I2C", xParameterStringLength) ==
                   0) {
            g_sIomConfig[port].eInterfaceMode = AM_HAL_IOM_I2C_MODE;
        }
    }

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 4, &xParameterStringLength);
    if (pcParameterString == NULL) {
        g_sIomConfig[port].ui32ClockFreq = AM_HAL_IOM_4MHZ;
    } else {
        g_sIomConfig[port].ui32ClockFreq = atoi(pcParameterString);
    }

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 5, &xParameterStringLength);
    if (pcParameterString == NULL) {
        g_sIomConfig[port].eSpiMode = AM_HAL_IOM_SPI_MODE_0;
    } else {
        g_sIomConfig[port].eSpiMode = atoi(pcParameterString);
    }

    am_hal_iom_initialize(port, &g_sIomHandler[port]);
    if (g_sIomHandler[port] == NULL) {
        strcat(pcWriteBuffer,
               "error: unable to open port or port number not supported.\r\n");
        return;
    }

    am_hal_iom_power_ctrl(g_sIomHandler[port], AM_HAL_SYSCTRL_WAKE, false);
    am_hal_iom_configure(g_sIomHandler[port], &g_sIomConfig[port]);
    am_bsp_iom_pins_enable(port, g_sIomConfig[port].eInterfaceMode);
    am_hal_iom_enable(g_sIomHandler[port]);

    strcat(pcWriteBuffer, "success: port ");
    strncat(pcWriteBuffer, pcPortString, 1);
    strcat(pcWriteBuffer, " opened.\r\n");
}

static void IomCloseSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString)
{
    const char *pcParameterString;
    const char *pcPortString;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    int port = -1;
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing port number\r\n");
        return;
    } else {
        pcPortString = pcParameterString;
        port = atoi(pcParameterString);
    }

    if (g_sIomHandler[port] == NULL) {
        strcat(pcWriteBuffer,
               "error: unable to close port or port is not open.\r\n");
        return;
    }
    am_hal_iom_power_ctrl(g_sIomHandler[port], AM_HAL_SYSCTRL_DEEPSLEEP, false);
    am_bsp_iom_pins_disable(port, g_sIomConfig[port].eInterfaceMode);
    am_hal_iom_disable(g_sIomHandler[port]);

    strcat(pcWriteBuffer, "success: port ");
    strncat(pcWriteBuffer, pcPortString, 1);
    strcat(pcWriteBuffer, " closed.\r\n");
}

static void IomWriteSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    int port = -1;
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: port unavailable.\r\n");
        return;
    } else {
        port = atoi(pcParameterString);
    }
    if (g_sIomHandler[port] == NULL) {
        strcat(pcWriteBuffer, "error: port opening port.\r\n");
        return;
    }

    size_t length = 0;
    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 3, &xParameterStringLength);
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: insufficient arguments.\r\n");
        return;
    } else {
        length = atoi(pcParameterString);
    }

    uint8_t buffer[BUFFER_MAX_SIZE];
    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 4, &xParameterStringLength);
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: insufficient arguments.\r\n");
        return;
    } else {
        IomFormatData(pcParameterString, xParameterStringLength, buffer,
                      &length);
    }

    am_hal_iom_transfer_t sTransaction = {
        .ui32InstrLen = 0,
        .ui32Instr = 0,
        .eDirection = AM_HAL_IOM_TX,
        .ui32NumBytes = length,
        .pui32TxBuffer = (uint32_t *)buffer,
        .bContinue = false,
        .ui8RepeatCount = 0,
        .ui32PauseCondition = 0,
        .ui32StatusSetClr = 0,
        .uPeerInfo.ui32SpiChipSelect = am_bsp_psSpiChipSelect[port],
    };

    if (am_hal_iom_blocking_transfer(g_sIomHandler[port], &sTransaction) ==
        AM_HAL_STATUS_SUCCESS) {
        strcat(pcWriteBuffer, "Wrote: ");
        strncat(pcWriteBuffer, pcParameterString, xParameterStringLength);
        strcat(pcWriteBuffer, "\r\n");
    } else {
        strcat(pcWriteBuffer, "error: unable to send data\r\n");
    }
}

static void IomReadSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                              const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    int port = -1;
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: port unavailable.\r\n");
        return;
    } else {
        port = atoi(pcParameterString);
    }
    if (g_sIomHandler[port] == NULL) {
        strcat(pcWriteBuffer, "error: port not open.\r\n");
        return;
    }

    size_t length = 0;
    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 3, &xParameterStringLength);
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing read length\r\n");
        return;
    } else {
        length = atoi(pcParameterString);
    }

    am_hal_iom_transfer_t sTransaction = {
        .ui32InstrLen = 0,
        .ui32Instr = 0,
        .bContinue = false,
        .ui8RepeatCount = 0,
        .ui32PauseCondition = 0,
        .ui32StatusSetClr = 0,
        .uPeerInfo.ui32SpiChipSelect = am_bsp_psSpiChipSelect[port],
    };

    uint8_t buffer[BUFFER_MAX_SIZE];
    memset(buffer, 0, length);
    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 4, &xParameterStringLength);
    if (pcParameterString != NULL) {
        size_t oplen = 0;
        uint8_t opcode[OPCODE_MAX_SIZE];
        memset(opcode, 0, OPCODE_MAX_SIZE);
        IomFormatData(pcParameterString, xParameterStringLength, opcode,
                      &oplen);

        buffer[0] = oplen;
        memcpy(&buffer[1], opcode, oplen);

        sTransaction.eDirection = AM_HAL_IOM_TX;
        sTransaction.ui32NumBytes = oplen;
        sTransaction.pui32TxBuffer = (uint32_t *)buffer;
        am_hal_iom_blocking_transfer(g_sIomHandler[port], &sTransaction);
    }

    sTransaction.eDirection = AM_HAL_IOM_RX;
    sTransaction.ui32NumBytes = length;
    sTransaction.pui32RxBuffer = (uint32_t *)buffer;

    am_hal_iom_blocking_transfer(g_sIomHandler[port], &sTransaction);

    char num[16];
    strcat(pcWriteBuffer, "Read:\r\n");
    size_t i = 0;
    for (i = 0; i < length - 1; i++) {
        am_util_stdio_sprintf(num, "0x%X, ", buffer[i]);
        strcat(pcWriteBuffer, num);
    }
    am_util_stdio_sprintf(num, "0x%X", buffer[i]);
    strcat(pcWriteBuffer, num);

    strcat(pcWriteBuffer, "\r\n");
}

portBASE_TYPE prvIomCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                            const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength, xReturn;

    pcWriteBuffer[0] = 0x0;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
    if (pcParameterString == NULL) {
        IomHelp(pcWriteBuffer);
    }

    if (strncmp(pcParameterString, "help", xParameterStringLength) == 0) {
        IomHelpSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "open", xParameterStringLength) ==
               0) {
        IomOpenSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "close", xParameterStringLength) ==
               0) {
        IomCloseSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "write", xParameterStringLength) ==
               0) {
        IomWriteSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "read", xParameterStringLength) ==
               0) {
        IomReadSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    }

    xReturn = pdFALSE;
    return xReturn;
}

void nm_iom_task(void *pvp)
{
    FreeRTOS_CLIRegisterCommand(&prvIomCommandDefinition);
    vTaskDelete(NULL);
}
