/*
 * iom.c
 *
 *  Created on: Jul 20, 2019
 *      Author: joshua
 */

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

//*****************************************************************************
//
// Standard AmbiqSuite includes.
//
//*****************************************************************************
#include <am_mcu_apollo.h>
#include <am_bsp.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <task.h>

#include "iom.h"

TaskHandle_t iom_task_handle;

#define BUFFER_MAX_SIZE 64
#define OPCODE_MAX_SIZE 8

static void *g_sIomHandler[AM_REG_IOM_NUM_MODULES];
static am_hal_iom_config_t g_sIomConfig[AM_REG_IOM_NUM_MODULES];

portBASE_TYPE prvIomCommand(char * pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString);

const CLI_Command_Definition_t prvIomCommandDefinition =
{
	(const char * const) "iom",
	(const char * const)
		"iom:\tApollo IO module configuration and control.\r\n"
	,
	prvIomCommand,
	-1
};

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

static void IomFormatData(const char *in, size_t inlen, uint8_t *out, size_t *outlen)
{
	size_t n = 0;
	char cNum[3];
	*outlen = 0;
	while (n < inlen)
	{
		switch (in[n])
		{
		case '\\':
			n++;
			switch(in[n])
			{
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

static void IomHelpSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	const char *pcParameterString;
	portBASE_TYPE xParameterStringLength;

	pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

	if (pcParameterString == NULL)
	{
		IomHelp(pcWriteBuffer);
		return;
	}

	if (strncmp(pcParameterString, "open", xParameterStringLength) == 0)
	{
		strcat(pcWriteBuffer, "usage: iom open [port number]\r\n");
	}
	else if (strncmp(pcParameterString, "write", xParameterStringLength) == 0)
	{
		strcat(pcWriteBuffer, "usage: iom write [port number] [data length] [data]\r\n");
	}
	else if (strncmp(pcParameterString, "read", xParameterStringLength) == 0)
	{
		strcat(pcWriteBuffer, "usage: iom read [port number] [bytes to read] [opcode]\r\n");
	}
}

static void IomOpenSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	const char *pcParameterString;
	portBASE_TYPE xParameterStringLength;

	pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

	int port = -1;
	if (pcParameterString == NULL)
		port = -1;
	else
		port = atoi(pcParameterString);

	switch (port)
	{
	/*
	case 0:
		xSPIPort[port] = FreeRTOS_open(SPI0, CMD_TIMEOUT);
		break;
	case 1:
		xSPIPort[port] = FreeRTOS_open(SPI1, CMD_TIMEOUT);
		break;
	case 2:
		xSPIPort[port] = FreeRTOS_open(SPI2, CMD_TIMEOUT);
		break;
	case 3:
		xSPIPort[port] = FreeRTOS_open(SPI3, CMD_TIMEOUT);
		break;
	case 5:
		xSPIPort[port] = FreeRTOS_open(SPI5, CMD_TIMEOUT);
		break;
		*/
	default:
		strcat(pcWriteBuffer, "error: unable to open port or port number not supported.\r\n");
		return;
	}

	strcat(pcWriteBuffer, "success: port ");
	strncat(pcWriteBuffer, pcParameterString, 1);
	strcat(pcWriteBuffer, " opened.\r\n");
}

static void IomWriteSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	const char *pcParameterString;
	portBASE_TYPE xParameterStringLength;

	pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

	int port = -1;
	if (pcParameterString == NULL)
	{
		strcat(pcWriteBuffer, "error: port unavailable.\r\n");
		return;
	}
	else
	{
		port = atoi(pcParameterString);
	}
	if (g_sIomHandler[port] == NULL)
	{
		strcat(pcWriteBuffer, "error: port opening port.\r\n");
		return;
	}

	size_t length = 0;
	pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 3, &xParameterStringLength);
	if (pcParameterString == NULL)
	{
		strcat(pcWriteBuffer, "error: insufficient arguments.\r\n");
		return;
	}
	else
	{
		length = atoi(pcParameterString);
	}

	uint8_t buffer[BUFFER_MAX_SIZE];
	pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 4, &xParameterStringLength);
	if (pcParameterString == NULL)
	{
		strcat(pcWriteBuffer, "error: insufficient arguments.\r\n");
		return;
	}
	else
	{
		IomFormatData(pcParameterString, xParameterStringLength, buffer, &length);
	}
/*
	if (FreeRTOS_ioctl(xSPIPort[port], ioctlOBTAIN_WRITE_MUTEX, CMD_TIMEOUT) == pdPASS)
    {
    	FreeRTOS_write(xSPIPort[port], buffer, length);
    }
*/
	strcat(pcWriteBuffer, "Wrote: ");
	strncat(pcWriteBuffer, pcParameterString, xParameterStringLength);
	strcat(pcWriteBuffer, "\r\n");
}

static void IomReadSubcommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	const char *pcParameterString;
	portBASE_TYPE xParameterStringLength;

	pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 2, &xParameterStringLength);

	int port = -1;
	if (pcParameterString == NULL)
	{
		strcat(pcWriteBuffer, "error: port unavailable.\r\n");
		return;
	}
	else
	{
		port = atoi(pcParameterString);
	}
	if (g_sIomHandler[port] == NULL)
	{
		strcat(pcWriteBuffer, "error: port not open.\r\n");
		return;
	}


	size_t length = 0;
	pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 3, &xParameterStringLength);
	if (pcParameterString == NULL)
	{
		strcat(pcWriteBuffer, "error: missing read length\r\n");
		return;
	}
	else
	{
		length = atoi(pcParameterString);
	}

	uint8_t buffer[BUFFER_MAX_SIZE];
	memset(buffer, 0, length);
	pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 4, &xParameterStringLength);
	if (pcParameterString == NULL)
	{
//		FreeRTOS_read(xSPIPort[port], buffer, length);
	}
	else
	{
		size_t oplen = 0;
		uint8_t opcode[OPCODE_MAX_SIZE];
		memset(opcode, 0, OPCODE_MAX_SIZE);
		IomFormatData(pcParameterString, xParameterStringLength, opcode, &oplen);

		buffer[0] = oplen;
		memcpy(&buffer[1], opcode, oplen);
//		FreeRTOS_read(xSPIPort[port], buffer, length);
	}

	uint8_t num[16];
	strcat(pcWriteBuffer, "Read:\r\n");
	size_t i = 0;
	for (i = 0; i < length - 1; i++)
	{
		am_util_stdio_sprintf(num, "0x%X, ", buffer[i]);
		strcat(pcWriteBuffer, num);
	}
	am_util_stdio_sprintf(num, "0x%X", buffer[i]);
	strcat(pcWriteBuffer, num);

	strcat(pcWriteBuffer, "\r\n");
}

portBASE_TYPE prvIomCommand(char *pcWriteBuffer, size_t xWriteBufferLen, const char *pcCommandString)
{
	const char *pcParameterString;
	portBASE_TYPE xParameterStringLength, xReturn;

	pcWriteBuffer[0] = 0x0;

	pcParameterString = FreeRTOS_CLIGetParameter(pcCommandString, 1, &xParameterStringLength);
	if (pcParameterString == NULL)
	{
		IomHelp(pcWriteBuffer);
	}

	if (strncmp(pcParameterString, "help", xParameterStringLength) == 0)
	{
		IomHelpSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
	}
	else if (strncmp(pcParameterString, "open", xParameterStringLength) == 0)
	{
		IomOpenSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
	}
	else if (strncmp(pcParameterString, "write", xParameterStringLength) == 0)
	{
		IomWriteSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
	}
	else if (strncmp(pcParameterString, "read", xParameterStringLength) == 0)
	{
		IomReadSubcommand(pcWriteBuffer, xWriteBufferLen, pcCommandString);
	}

	xReturn = pdFALSE;
	return xReturn;
}

void g_iom_task(void *pvp)
{
	FreeRTOS_CLIRegisterCommand(&prvIomCommandDefinition);
	vTaskDelete(NULL);
}
