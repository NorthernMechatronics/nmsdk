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

#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>

#include "gpio_service.h"

TaskHandle_t nm_gpio_task_handle;

#define MAX_GPIO_PINS 50

static uint8_t gReservedPins[] = {
    20, 21, 41,                    // SWD
    22, 23, 33, 37,                // J-Link VCOM
    16, 18, 19,                    // Push-Buttons
    36, 38, 39, 40, 42, 43, 44, 47 // SX1262
};

static size_t MAX_RESERVED_PINS = sizeof(gReservedPins);

portBASE_TYPE prvGpioCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                             const char *pcCommandString);

const CLI_Command_Definition_t prvGpioCommandDefinition = {
    (const char *const) "gpio",
    (const char *const) "gpio:\tApollo gpio control.\r\n", prvGpioCommand, -1};

static void GpioHelp(char *pcWriteBuffer)
{
    strcat(pcWriteBuffer, "usage: gpio <command> [<args>]\r\n");
    strcat(pcWriteBuffer, "\r\n");
    strcat(pcWriteBuffer, "Supported commands are:\r\n");
    strcat(pcWriteBuffer, "  init\t\tinitialize all GPIOs\r\n");
    strcat(pcWriteBuffer, "  deinit\tde-initialize all GPIOs\r\n");
    strcat(pcWriteBuffer, "  write\t\twrite state to a GPIO pin\r\n");
    strcat(pcWriteBuffer, "  read\t\tread state from a GPIO pin\r\n");
    strcat(pcWriteBuffer, "  help\t\tshow command details\r\n");
    strcat(pcWriteBuffer, "\r\n");
    strcat(pcWriteBuffer, "See 'gpio help <command>' for command details.\r\n");
}

static void GpioHelpSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    if (pcParameterString == NULL) {
        GpioHelp(pcWriteBuffer);
        return;
    }

    if (strncmp(pcParameterString, "init", xParameterStringLength) == 0) {
        strcat(pcWriteBuffer, "usage: gpio init [function] <pin number>\r\n");
        strcat(pcWriteBuffer, "\t[function] is either output or input\r\n");
        strcat(pcWriteBuffer,
               "\t<pin number> is one of the available GPIO pins\r\n");
    } else if (strncmp(pcParameterString, "deinit", xParameterStringLength) ==
               0) {
        strcat(pcWriteBuffer, "usage: gpio deinit <pin number>\r\n");
        strcat(pcWriteBuffer,
               "\t<pin number> is one of the available GPIO pins\r\n");
    } else if (strncmp(pcParameterString, "write", xParameterStringLength) ==
               0) {
        strcat(pcWriteBuffer, "usage: gpio write [pin number] [state]\r\n");
    } else if (strncmp(pcParameterString, "read", xParameterStringLength) ==
               0) {
        strcat(pcWriteBuffer, "usage: gpio read [pin number]\r\n");
    } else if (strncmp(pcParameterString, "toggle", xParameterStringLength) ==
               0) {
        strcat(pcWriteBuffer, "usage: gpio toggle [pin number]\r\n");
    }
}

static bool GpioIsReserved(uint32_t pin)
{
    for (int i = 0; i < MAX_RESERVED_PINS; i++) {
        if (gReservedPins[i] == pin)
            return true;
    }

    return false;
}

static bool GpioInitOutput(uint32_t pin)
{
    if (GpioIsReserved(pin)) {
        return false;
    }

    if (pin < MAX_GPIO_PINS) {
        am_hal_gpio_pinconfig(pin, g_AM_HAL_GPIO_OUTPUT);

        am_hal_gpio_state_write(pin, AM_HAL_GPIO_OUTPUT_TRISTATE_DISABLE);
        am_hal_gpio_state_write(pin, AM_HAL_GPIO_OUTPUT_CLEAR);
    } else {
        for (int i = 0; i < MAX_GPIO_PINS; i++) {
            if (!GpioIsReserved(i)) {
                am_hal_gpio_pinconfig(i, g_AM_HAL_GPIO_OUTPUT);

                am_hal_gpio_state_write(i, AM_HAL_GPIO_OUTPUT_TRISTATE_DISABLE);
                am_hal_gpio_state_write(i, AM_HAL_GPIO_OUTPUT_CLEAR);
            }
        }
    }

    return true;
}

static bool GpioInitInput(uint32_t pin)
{
    if (GpioIsReserved(pin)) {
        return false;
    }

    if (pin < MAX_GPIO_PINS) {
        am_hal_gpio_pinconfig(pin, g_AM_HAL_GPIO_INPUT);
    } else {
        for (int i = 0; i < MAX_GPIO_PINS; i++) {
            if (!GpioIsReserved(i)) {
                am_hal_gpio_pinconfig(i, g_AM_HAL_GPIO_INPUT);
            }
        }
    }

    return true;
}

static bool GpioDeinit(uint32_t pin)
{
    if (GpioIsReserved(pin)) {
        return false;
    }

    if (pin < MAX_GPIO_PINS) {
        am_hal_gpio_pinconfig(pin, g_AM_HAL_GPIO_DISABLE);
    } else {
        for (int i = 0; i < MAX_GPIO_PINS; i++) {
            if (!GpioIsReserved(i)) {
                am_hal_gpio_pinconfig(i, g_AM_HAL_GPIO_DISABLE);
            }
        }
    }

    return true;
}

static void GpioInitSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);
    if (strncmp(pcParameterString, "output", xParameterStringLength) == 0) {
        pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                                                     &xParameterStringLength);
        if (pcParameterString == NULL) {
            GpioInitOutput(MAX_GPIO_PINS);
            strcat(pcWriteBuffer,
                   "Configured all unreserved GPIO pins to output.\r\n");
        } else {
            uint32_t ui32Pin = atoi(pcParameterString);
            if (!GpioInitOutput(ui32Pin)) {
                strcat(pcWriteBuffer, "error: GPIO ");
                strncat(pcWriteBuffer, pcParameterString,
                        xParameterStringLength);
                strcat(pcWriteBuffer, " is a reserved pin.\r\n");
            } else {
                strcat(pcWriteBuffer, "Configured GPIO ");
                strncat(pcWriteBuffer, pcParameterString,
                        xParameterStringLength);
                strcat(pcWriteBuffer, " as an output.\r\n");
            }
        }
    } else if (strncmp(pcParameterString, "input", xParameterStringLength) ==
               0) {
        pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 3,
                                                     &xParameterStringLength);
        if (pcParameterString == NULL) {
            GpioInitInput(MAX_GPIO_PINS);
            strcat(pcWriteBuffer,
                   "Configured all unreserved GPIO pins to input.\r\n");
        } else {
            uint32_t ui32Pin = atoi(pcParameterString);
            if (!GpioInitInput(ui32Pin)) {
                strcat(pcWriteBuffer, "error: GPIO ");
                strncat(pcWriteBuffer, pcParameterString,
                        xParameterStringLength);
                strcat(pcWriteBuffer, " is a reserved pin.\r\n");
            } else {
                strcat(pcWriteBuffer, "Configured GPIO ");
                strncat(pcWriteBuffer, pcParameterString,
                        xParameterStringLength);
                strcat(pcWriteBuffer, " as an input.\r\n");
            }
        }
    }
}

static void GpioDeinitSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                 const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

    if (pcParameterString == NULL) {
        GpioDeinit(MAX_GPIO_PINS);
        strcat(pcWriteBuffer, "All unreserved GPIO pins are disabled.\r\n");
    } else {
        uint32_t ui32Pin = atoi(pcParameterString);
        if (!GpioDeinit(ui32Pin)) {
            strcat(pcWriteBuffer, "error: GPIO ");
            strncat(pcWriteBuffer, pcParameterString, xParameterStringLength);
            strcat(pcWriteBuffer, " is a reserved pin.\r\n");
        } else {
            strcat(pcWriteBuffer, "GPIO ");
            strncat(pcWriteBuffer, pcParameterString, xParameterStringLength);
            strcat(pcWriteBuffer, " is disabled.\r\n");
        }
    }
}

static void GpioWriteSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;
    uint32_t ui32Pin, ui32State;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing pin number\r\n");
        return;
    } else {
        ui32Pin = atoi(pcParameterString);
        if (GpioIsReserved(ui32Pin)) {
            strcat(pcWriteBuffer, "error: GPIO ");
            strncat(pcWriteBuffer, pcParameterString, xParameterStringLength);
            strcat(pcWriteBuffer, " is a reserved pin.\r\n");
        }
    }

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 3, &xParameterStringLength);
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing pin state\r\n");
        return;
    } else {
        ui32State = atoi(pcParameterString);
    }

    am_hal_gpio_state_write(ui32Pin, ui32State);
}

static void GpioReadSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                               const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;

    uint32_t ui32Pin, ui32State;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing pin number\r\n");
        return;
    } else {
        ui32Pin = atoi(pcParameterString);
        if (GpioIsReserved(ui32Pin)) {
            strcat(pcWriteBuffer, "error: GPIO ");
            strncat(pcWriteBuffer, pcParameterString, xParameterStringLength);
            strcat(pcWriteBuffer, " is a reserved pin.\r\n");
        }
    }

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 3, &xParameterStringLength);
    if (pcParameterString == NULL) {
        am_hal_gpio_state_read(ui32Pin, AM_HAL_GPIO_INPUT_READ, &ui32State);
    } else if (strncmp(pcParameterString, "output", xParameterStringLength) ==
               0) {
        am_hal_gpio_state_read(ui32Pin, AM_HAL_GPIO_OUTPUT_READ, &ui32State);
    } else {
        strcat(pcWriteBuffer, "error: unknown read type\r\n");
        return;
    }

    strcat(pcWriteBuffer, "GPIO ");
    strncat(pcWriteBuffer, pcParameterString, xParameterStringLength);
    strcat(pcWriteBuffer, " state:");

    char *buffer = pcWriteBuffer + strlen(pcWriteBuffer);
    am_util_stdio_sprintf(buffer, " %d\r\n", ui32State);
}

static void GpioToggleSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                                 const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength;
    uint32_t ui32Pin;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);
    if (pcParameterString == NULL) {
        strcat(pcWriteBuffer, "error: missing pin number\r\n");
        return;
    } else {
        ui32Pin = atoi(pcParameterString);
        if (GpioIsReserved(ui32Pin)) {
            strcat(pcWriteBuffer, "error: GPIO ");
            strncat(pcWriteBuffer, pcParameterString, xParameterStringLength);
            strcat(pcWriteBuffer, " is a reserved pin.\r\n");
        }
    }

    am_hal_gpio_state_write(ui32Pin, AM_HAL_GPIO_OUTPUT_TOGGLE);
}

portBASE_TYPE prvGpioCommand(char *pcWriteBuffer, size_t xWriteBufferLen,
                             const char *pcCommandString)
{
    const char *pcParameterString;
    portBASE_TYPE xParameterStringLength, xReturn;

    pcWriteBuffer[0] = 0x0;

    pcParameterString =
        FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
    if (pcParameterString == NULL) {
        GpioHelp(pcWriteBuffer);
        return pdFALSE;
    }

    if (strncmp(pcParameterString, "help", xParameterStringLength) == 0) {
        GpioHelpSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "init", xParameterStringLength) ==
               0) {
        GpioInitSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "deinit", xParameterStringLength) ==
               0) {
        GpioDeinitSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "write", xParameterStringLength) ==
               0) {
        GpioWriteSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "read", xParameterStringLength) ==
               0) {
        GpioReadSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    } else if (strncmp(pcParameterString, "toggle", xParameterStringLength) ==
               0) {
        GpioToggleSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
    }

    xReturn = pdFALSE;
    return xReturn;
}

void nm_gpio_task(void *pvp)
{
    FreeRTOS_CLIRegisterCommand(&prvGpioCommandDefinition);
    vTaskDelete(NULL);
}
