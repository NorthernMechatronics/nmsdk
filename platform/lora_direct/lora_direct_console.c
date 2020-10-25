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
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <am_util.h>

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <queue.h>
#include <task.h>

#include <nm_devices_lora.h>

#include "console_task.h"

#include "lora_direct_config.h"
#include "lora_direct_console.h"
#include "lora_direct_task.h"

TaskHandle_t lora_direct_console_task_handle;

portBASE_TYPE prvLoRaCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                             const char *pcCommandString);

const CLI_Command_Definition_t prvLoRaCommandDefinition = {
    (const char *const) "lora",
    (const char *const) "lora:\tLoRa radio command interface.\r\n",
    prvLoRaCommand, -1};

static void LoRaHelpSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "usage: lora <command> [<args>]\r\n");
        strcat(pcWriteBuffer, "\r\n");
        strcat(pcWriteBuffer, "Supported commands are:\r\n");
        strcat(pcWriteBuffer, "  init     initialize the LoRa radio\r\n");
        strcat(pcWriteBuffer, "  reset    reset the LoRa radio\r\n");
        strcat(pcWriteBuffer, "  txcw     continuous transmit\r\n");
        strcat(pcWriteBuffer, "  send     transmit message\r\n");
        strcat(pcWriteBuffer, "  rx       continuous receive\r\n");
        strcat(pcWriteBuffer, "  get      get LoRa radio parameters\r\n");
        strcat(pcWriteBuffer,
               "  display  show/hide received payload content\r\n");
        strcat(pcWriteBuffer, "  help     show command details\r\n");
        strcat(pcWriteBuffer, "\r\n");
        strcat(pcWriteBuffer,
               "See 'lora help <command>' for command details.\r\n");

    } else if (strncmp(pcParameterString, "init", 4) == 0) {
        strcat(pcWriteBuffer, "usage: lora init\r\n");
    } else if (strncmp(pcParameterString, "deinit", 6) == 0) {
        strcat(pcWriteBuffer, "usage: lora deinit\r\n");
    } else if (strncmp(pcParameterString, "reset", 5) == 0) {
        strcat(pcWriteBuffer, "usage: lora reset\r\n");
    } else if (strncmp(pcParameterString, "txcw", 4) == 0) {
        strcat(pcWriteBuffer, "usage: lora txcw [freq] [power]\r\n");
        strcat(pcWriteBuffer, "  freq  is in MHz, real scalar\r\n");
        strcat(pcWriteBuffer, "  power is in dBm, integer scalar\r\n");
    } else if (strncmp(pcParameterString, "send", 4) == 0) {
        strcat(pcWriteBuffer, "usage: lora send <freq> <power> [msg]\r\n");
        strcat(pcWriteBuffer, "  msg   is a string\r\n\r\n");
        strcat(pcWriteBuffer, "optional:\r\n");
        strcat(pcWriteBuffer, "  freq  is in MHz, real scalar\r\n");
        strcat(pcWriteBuffer, "  power is in dBm, integer scalar\r\n\r\n");
        strcat(pcWriteBuffer,
               "Note: freq and power must be specified together.\r\n");
    } else if (strncmp(pcParameterString, "rx", 2) == 0) {
        strcat(pcWriteBuffer, "usage: lora rx [freq]\r\n");
        strcat(pcWriteBuffer, "  freq is in MHz, real scalar\r\n");
    } else if (strncmp(pcParameterString, "get", 3) == 0) {
        strcat(pcWriteBuffer, "usage: lora get [parameter]\r\n\r\n");
        strcat(pcWriteBuffer, "valid parameter are:\r\n");
        strcat(pcWriteBuffer, "  freq   communication frequeny\r\n");
        strcat(pcWriteBuffer, "  power  transmit power\r\n");
        strcat(pcWriteBuffer, "  sf     spreading factor\r\n");
        strcat(pcWriteBuffer, "  bw     bandwidth\r\n");
        strcat(pcWriteBuffer, "  cr     coding rate\r\n");
        strcat(pcWriteBuffer, "  sync   sync word\r\n");
    } else if (strncmp(pcParameterString, "set", 3) == 0) {
        strcat(pcWriteBuffer, "usage: lora set [parameter] [value]\r\n\r\n");
        strcat(pcWriteBuffer, "valid parameter are:\r\n");
        strcat(pcWriteBuffer, "  freq   communication frequeny\r\n");
        strcat(pcWriteBuffer, "  power  transmit power\r\n");
        strcat(pcWriteBuffer, "  sf     spreading factor\r\n");
        strcat(pcWriteBuffer, "  bw     bandwidth\r\n");
        strcat(pcWriteBuffer, "  cr     coding rate\r\n");
        strcat(pcWriteBuffer, "  sync   sync word\r\n");
    } else {
        strcat(pcWriteBuffer, "unknown command option\r\n");
    }
}

static void LoRaInitSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString)
{
    lora_direct_radio_configuration_reset();
    lora_radio_initialize(NULL);
    strcat(pcWriteBuffer, "LoRa radio initialized\r\n");
}

static void LoRaDeinitSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                 const char *pcCommandString)
{
    lora_radio_power_ctrl(NULL, LORA_RADIO_DEEPSLEEP);
    lora_radio_deinitialize(NULL);
    strcat(pcWriteBuffer, "LoRa radio de-initialized\r\n");
}

static void LoRaResetSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString)
{
    lora_radio_reset(NULL);
    strcat(pcWriteBuffer, "LoRa radio reset\r\n");
}

static void LoRaTxcwSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString)
{
    const char *pcParameterString = NULL, *pcTransmitPower = NULL;
    float f;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    uint32_t freq = 0;
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing frequency\r\n");
        return;
    } else {
        pcTransmitPower = pcParameterString + xParameterStringLength + 1;

        char buffer[12];
        strncpy(buffer, pcParameterString, xParameterStringLength);
        f = atof(buffer);
        freq = (uint32_t)(f * 1e6);
    }

    uint32_t power = 0;
    if (pcTransmitPower == NULL) {
        strcat(pcWriteBuffer, "error: missing power\r\n");
        return;
    } else {
        power = atoi(pcTransmitPower);
    }

    lora_direct_transmit_carrier(freq, power);

    char *buffer = pcWriteBuffer + strlen(pcWriteBuffer);
    am_util_stdio_sprintf(buffer, "LoRa carrier wave: %0.2f MHz at %d dBm\r\n",
                          f, power);
}

static void LoRaSendSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString)
{
    const char *pcParameterString = NULL, *pcTransmitPower = NULL;
    portBASE_TYPE xParameterStringLength;
    double f;

    char *buffer = pcWriteBuffer + strlen(pcWriteBuffer);
    int8_t argc = FreeRTOS_CLIGetNumberOfParameters(pcCommandString);

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    if (argc == 2) {
        lora_direct_send(lora_radio_frequency, lora_radio_power,
                         (const uint8_t *)pcParameterString,
                         xParameterStringLength);
        am_util_stdio_sprintf(buffer,
                              "\r\nTransmit Parameters:\r\n%0.2f MHz at %d "
                              "dBm\r\n\r\nPayload:\r\n",
                              lora_radio_frequency / 1e6, lora_radio_power);
    } else {

        uint32_t freq = 0;
        if (pcParameterString == NULL) {
            strcat(pcWriteBuffer, "error: missing frequency\r\n");
            return;
        } else {
            pcTransmitPower = pcParameterString + xParameterStringLength + 1;

            char fbuffer[12];
            strncpy(fbuffer, pcParameterString, xParameterStringLength);
            f = atof(fbuffer);
            freq = (uint32_t)(f * 1e6);
        }

        uint32_t power = 0;
        if (pcTransmitPower == NULL) {
            strcat(pcWriteBuffer, "error: missing power\r\n");
            return;
        } else {
            power = atoi(pcTransmitPower);
        }

        pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 4,
                                                     &xParameterStringLength);
        if (pcParameterString == NULL) {
            strcat(pcWriteBuffer, "error: missing message\r\n");
            return;
        }

        lora_direct_send(freq, power, (const uint8_t *)pcParameterString,
                         xParameterStringLength);

        am_util_stdio_sprintf(buffer,
                              "\r\nTransmit Parameters:\r\n%0.2f MHz at %d "
                              "dBm\r\n\r\nPayload:\r\n",
                              f, power);
    }
    strncat(buffer, pcParameterString, xParameterStringLength);
    strcat(buffer, "\r\n");
}

static void LoRaRxSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                             const char *pcCommandString)
{
    const char *pcParameterString = NULL;
    portBASE_TYPE xParameterStringLength;
    double f;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    uint32_t freq = 0;
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing frequency\r\n");
        return;
    } else {
        char buffer[12];
        strncpy(buffer, pcParameterString, xParameterStringLength);
        f = atof(buffer);
        freq = (uint32_t)(f * 1e6);
    }

    lora_direct_receive(freq);

    char *buffer = pcWriteBuffer + strlen(pcWriteBuffer);
    am_util_stdio_sprintf(buffer, "LoRa radio listening at %0.2f MHz\r\n", f);
}

static void LoRaSetSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                              const char *pcCommandString)
{
    const char *pcParameterString = NULL;
    portBASE_TYPE xParameterStringLength;
    double f;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing parameter\r\n");
        return;
    }

    char *buffer = pcWriteBuffer + strlen(pcWriteBuffer);
    if (strncmp(pcParameterString, LORA_RADIO_FREQUENCY,
                xParameterStringLength) == 0) {
        pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                                                     &xParameterStringLength);

        if (pcParameterString == NULL) {
            strcat(pcWriteBuffer, "error: missing frequency\r\n");
            return;
        } else {
            char fbuffer[12];
            strncpy(fbuffer, pcParameterString, xParameterStringLength);
            f = atof(fbuffer);
            lora_radio_frequency = (uint32_t)(f * 1e6);
        }

        am_util_stdio_sprintf(buffer, "\r\nSet frequency to %0.2f MHz\r\n",
                              lora_radio_frequency / 1e6);
    } else if (strncmp(pcParameterString, LORA_RADIO_POWER,
                       xParameterStringLength) == 0) {
        pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                                                     &xParameterStringLength);

        if (pcParameterString == NULL) {
            strcat(pcWriteBuffer, "error: missing frequency\r\n");
            return;
        } else {
            lora_radio_power = atoi(pcParameterString);
        }

        am_util_stdio_sprintf(buffer, "\r\n%0d dBm\r\n", lora_radio_power);
    } else if (strncmp(pcParameterString, LORA_RADIO_SPREADING_FACTOR,
                       xParameterStringLength) == 0) {
        pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                                                     &xParameterStringLength);

        if (pcParameterString == NULL) {
            strcat(pcWriteBuffer, "error: missing spreading factor\r\n");
            strcat(pcWriteBuffer, "valid spreading factors are:\r\n");
            strcat(pcWriteBuffer, "   5\r\n");
            strcat(pcWriteBuffer, "   6\r\n");
            strcat(pcWriteBuffer, "   7\r\n");
            strcat(pcWriteBuffer, "   8\r\n");
            strcat(pcWriteBuffer, "   9\r\n");
            strcat(pcWriteBuffer, "  10\r\n");
            strcat(pcWriteBuffer, "  11\r\n");
            strcat(pcWriteBuffer, "  12\r\n");
            return;
        } else {
            gsLoRaModulationParameter.eSpreadingFactor =
                atoi(pcParameterString);
        }

        am_util_stdio_sprintf(buffer, "\r\nSpreading Factor: %d\r\n",
                              gsLoRaModulationParameter.eSpreadingFactor);
    } else if (strncmp(pcParameterString, LORA_RADIO_BANDWIDTH,
                       xParameterStringLength) == 0) {
        pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                                                     &xParameterStringLength);

        if (pcParameterString == NULL) {
            strcat(pcWriteBuffer, "error: missing bandwidth\r\n");
            strcat(pcWriteBuffer, "valid bandwidths are:\r\n");
            strcat(pcWriteBuffer, "    7\r\n");
            strcat(pcWriteBuffer, "   10\r\n");
            strcat(pcWriteBuffer, "   15\r\n");
            strcat(pcWriteBuffer, "   20\r\n");
            strcat(pcWriteBuffer, "   31\r\n");
            strcat(pcWriteBuffer, "   41\r\n");
            strcat(pcWriteBuffer, "   62\r\n");
            strcat(pcWriteBuffer, "  125\r\n");
            strcat(pcWriteBuffer, "  250\r\n");
            strcat(pcWriteBuffer, "  500\r\n");
            return;
        } else {
            int16_t bandwidth = atoi(pcParameterString);
            switch (bandwidth) {
            case 7:
                gsLoRaModulationParameter.eBandwidth = LORA_RADIO_BW_7;
                break;
            case 10:
                gsLoRaModulationParameter.eBandwidth = LORA_RADIO_BW_10;
                break;
            case 15:
                gsLoRaModulationParameter.eBandwidth = LORA_RADIO_BW_15;
                break;
            case 20:
                gsLoRaModulationParameter.eBandwidth = LORA_RADIO_BW_20;
                break;
            case 31:
                gsLoRaModulationParameter.eBandwidth = LORA_RADIO_BW_31;
                break;
            case 41:
                gsLoRaModulationParameter.eBandwidth = LORA_RADIO_BW_41;
                break;
            case 62:
                gsLoRaModulationParameter.eBandwidth = LORA_RADIO_BW_62;
                break;
            case 125:
                gsLoRaModulationParameter.eBandwidth = LORA_RADIO_BW_125;
                break;
            case 250:
                gsLoRaModulationParameter.eBandwidth = LORA_RADIO_BW_250;
                break;
            case 500:
                gsLoRaModulationParameter.eBandwidth = LORA_RADIO_BW_500;
                break;
            default:
                bandwidth = -1;
                break;
            }

            if (bandwidth == -1) {
                am_util_stdio_sprintf(buffer,
                                      "\r\nUnknown bandwidth specified.  "
                                      "Parameters unchanged.\r\n");
            } else {
                am_util_stdio_sprintf(buffer, "\r\nBandwidth: %d kHz\r\n",
                                      bandwidth);
            }
        }
    } else if (strncmp(pcParameterString, LORA_RADIO_CODING_RATE,
                       xParameterStringLength) == 0) {
        pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                                                     &xParameterStringLength);

        if (pcParameterString == NULL) {
            strcat(pcWriteBuffer, "error: missing redundancy\r\n");
            strcat(pcWriteBuffer, "valid redundancy are:\r\n");
            strcat(pcWriteBuffer, "  5\r\n");
            strcat(pcWriteBuffer, "  6\r\n");
            strcat(pcWriteBuffer, "  7\r\n");
            strcat(pcWriteBuffer, "  8\r\n");
            return;
        } else {

            int16_t redundancy = atoi(pcParameterString);
            switch (redundancy) {
            case 5:
                gsLoRaModulationParameter.eCodingRate = LORA_CR_4_5;
                break;
            case 6:
                gsLoRaModulationParameter.eCodingRate = LORA_CR_4_6;
                break;
            case 7:
                gsLoRaModulationParameter.eCodingRate = LORA_CR_4_7;
                break;
            case 8:
                gsLoRaModulationParameter.eCodingRate = LORA_CR_4_8;
                break;
            default:
                redundancy = -1;
                break;
            }

            if (redundancy == -1) {
                am_util_stdio_sprintf(buffer,
                                      "\r\nUnknown redundancy specified.  "
                                      "Parameters unchanged.\r\n");
            } else {
                am_util_stdio_sprintf(
                    buffer,
                    "\r\nCoding Rate: 4-bit data / %d-bit redundancy\r\n",
                    redundancy);
            }
        }
    } else {
        strcat(buffer, "\r\nunknown parameter specified\r\n");
    }

    lora_direct_receive(lora_radio_frequency);
}

static void LoRaGetSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                              const char *pcCommandString)
{
    const char *pcParameterString = NULL;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing parameter\r\n");
        return;
    }

    char *buffer = pcWriteBuffer + strlen(pcWriteBuffer);
    if (strncmp(pcParameterString, LORA_RADIO_FREQUENCY,
                xParameterStringLength) == 0) {
        am_util_stdio_sprintf(buffer, "\r\n%0.2f MHz\r\n",
                              lora_radio_frequency / 1e6);
    } else if (strncmp(pcParameterString, LORA_RADIO_POWER,
                       xParameterStringLength) == 0) {
        am_util_stdio_sprintf(buffer, "\r\n%0d dBm\r\n", lora_radio_power);
    } else if (strncmp(pcParameterString, LORA_RADIO_SPREADING_FACTOR,
                       xParameterStringLength) == 0) {
        am_util_stdio_sprintf(buffer, "\r\nSpreading Factor: %d\r\n",
                              gsLoRaModulationParameter.eSpreadingFactor);
    } else if (strncmp(pcParameterString, LORA_RADIO_BANDWIDTH,
                       xParameterStringLength) == 0) {
        uint16_t bandwidth = 0;
        switch (gsLoRaModulationParameter.eBandwidth) {
        case LORA_RADIO_BW_7:
            bandwidth = 7;
            break;
        case LORA_RADIO_BW_10:
            bandwidth = 10;
            break;
        case LORA_RADIO_BW_15:
            bandwidth = 15;
            break;
        case LORA_RADIO_BW_20:
            bandwidth = 20;
            break;
        case LORA_RADIO_BW_31:
            bandwidth = 31;
            break;
        case LORA_RADIO_BW_41:
            bandwidth = 41;
            break;
        case LORA_RADIO_BW_62:
            bandwidth = 62;
            break;
        case LORA_RADIO_BW_125:
            bandwidth = 125;
            break;
        case LORA_RADIO_BW_250:
            bandwidth = 250;
            break;
        case LORA_RADIO_BW_500:
            bandwidth = 500;
            break;
        }
        am_util_stdio_sprintf(buffer, "\r\nBandwidth: %dkHz\r\n", bandwidth);
    } else if (strncmp(pcParameterString, LORA_RADIO_CODING_RATE,
                       xParameterStringLength) == 0) {
        uint8_t redundancy = 0;
        switch (gsLoRaModulationParameter.eCodingRate) {
        case LORA_CR_4_5:
            redundancy = 5;
            break;
        case LORA_CR_4_6:
            redundancy = 6;
            break;
        case LORA_CR_4_7:
            redundancy = 7;
            break;
        case LORA_CR_4_8:
            redundancy = 8;
            break;
        }
        am_util_stdio_sprintf(
            buffer, "\r\nCoding Rate: 4-bit data / %d-bit redundancy\r\n",
            redundancy);
    } else if (strncmp(pcParameterString, LORA_RADIO_SYNCWORD,
                       xParameterStringLength) == 0) {
        am_util_stdio_sprintf(buffer, "\r\nSync Word: 0x%08X\r\n",
                              lora_radio_syncword);
    } else {
        strcat(buffer, "\r\nunknown parameter specified\r\n");
    }
}

portBASE_TYPE prvLoRaCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                             const char *pcCommandString)
{
    const char *pcParameterString = NULL;
    ;
    portBASE_TYPE xParameterStringLength;

    pcWriteBuffer[0] = 0x0;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
    if (pcParameterString == NULL) {
        LoRaHelpSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
        return pdFALSE;
    }

    if (strncmp(pcParameterString, "help", 4) == 0) {
        LoRaHelpSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "init", 4) == 0) {
        LoRaInitSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "deinit", 6) == 0) {
        LoRaDeinitSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "reset", 5) == 0) {
        LoRaResetSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "txcw", 4) == 0) {
        LoRaTxcwSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "send", 4) == 0) {
        LoRaSendSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "set", 3) == 0) {
        LoRaSetSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "get", 3) == 0) {
        LoRaGetSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "rx", 2) == 0) {
        LoRaRxSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    }

    return pdFALSE;
}

void lora_direct_console_task(void *pvp)
{
    FreeRTOS_CLIRegisterCommand(&prvLoRaCommandDefinition);
    vTaskDelete(NULL);
}
