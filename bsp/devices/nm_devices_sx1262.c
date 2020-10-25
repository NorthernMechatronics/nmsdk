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
#include <machine/endian.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include <am_bsp.h>
#include <am_mcu_apollo.h>
#include <am_util.h>

#include "nm_devices_lora.h"

#define RSSI_OFF 64
#define SNR_SCALEUP 4

#define CMD_SETSLEEP 0x84
#define CMD_SETSTANDBY 0x80
#define CMD_SETFS 0xC1
#define CMD_SETTX 0x83
#define CMD_SETRX 0x82
#define CMD_STOPTIMERONPREAMBLE 0x9F
#define CMD_SETRXDUTYCYCLE 0x94
#define CMD_SETCAD 0xC5
#define CMD_SETTXCONTINUOUSWAVE 0xD1
#define CMD_SETTXINFINITEPREAMBLE 0xD2
#define CMD_SETREGULATORMODE 0x96
#define CMD_CALIBRATE 0x89
#define CMD_CALIBRATEIMAGE 0x98
#define CMD_SETPACONFIG 0x95
#define CMD_SETRXTXFALLBACKMODE 0x93

// Commands to Access the Radio Registers and FIFO Buffer
#define CMD_WRITEREGISTER 0x0D
#define CMD_READREGISTER 0x1D
#define CMD_WRITEBUFFER 0x0E
#define CMD_READBUFFER 0x1E

// Commands Controlling the Radio IRQs and DIOs
#define CMD_SETDIOIRQPARAMS 0x08
#define CMD_GETIRQSTATUS 0x12
#define CMD_CLEARIRQSTATUS 0x02
#define CMD_SETDIO2ASRFSWITCHCTRL 0x9D
#define CMD_SETDIO3ASTCXOCTRL 0x97

// Commands Controlling the RF and Packets Settings
#define CMD_SETRFFREQUENCY 0x86
#define CMD_SETPACKETTYPE 0x8A
#define CMD_GETPACKETTYPE 0x11
#define CMD_SETTXPARAMS 0x8E
#define CMD_SETMODULATIONPARAMS 0x8B
#define CMD_SETPACKETPARAMS 0x8C
#define CMD_SETCADPARAMS 0x88
#define CMD_SETBUFFERBASEADDRESS 0x8F
#define CMD_SETLORASYMBNUMTIMEOUT 0xA0

// Commands Returning the Radio Status
#define CMD_GETSTATUS 0xC0
#define CMD_GETRSSIINST 0x15
#define CMD_GETRXBUFFERSTATUS 0x13
#define CMD_GETPACKETSTATUS 0x14
#define CMD_GETDEVICEERRORS 0x17
#define CMD_CLEARDEVICEERRORS 0x07
#define CMD_GETSTATS 0x10
#define CMD_RESETSTATS 0x00

// ----------------------------------------
// List of Registers

#define REG_WHITENINGMSB 0x06B8
#define REG_WHITENINGLSB 0x06B9
#define REG_CRCINITVALMSB 0x06BC
#define REG_CRCINITVALLSB 0x06BD
#define REG_CRCPOLYVALMSB 0x06BE
#define REG_CRCPOLYVALLSB 0x06BF
#define REG_SYNCWORD0 0x06C0
#define REG_SYNCWORD1 0x06C1
#define REG_SYNCWORD2 0x06C2
#define REG_SYNCWORD3 0x06C3
#define REG_SYNCWORD4 0x06C4
#define REG_SYNCWORD5 0x06C5
#define REG_SYNCWORD6 0x06C6
#define REG_SYNCWORD7 0x06C7
#define REG_NODEADDRESS 0x06CD
#define REG_BROADCASTADDR 0x06CE
#define REG_LORASYNCWORDMSB 0x0740
#define REG_LORASYNCWORDLSB 0x0741
#define REG_RANDOMNUMBERGEN0 0x0819
#define REG_RANDOMNUMBERGEN1 0x081A
#define REG_RANDOMNUMBERGEN2 0x081B
#define REG_RANDOMNUMBERGEN3 0x081C
#define REG_RXGAIN 0x08AC
#define REG_OCPCONFIG 0x08E7
#define REG_XTATRIM 0x0911
#define REG_XTBTRIM 0x0912

// sleep modes
#define SLEEP_COLD 0x00 // (no rtc timeout)
#define SLEEP_WARM 0x04 // (no rtc timeout)

// standby modes
#define STDBY_RC 0x00
#define STDBY_XOSC 0x01

// regulator modes
#define REGMODE_LDO 0x00
#define REGMODE_DCDC 0x01

// packet types
#define PACKET_TYPE_FSK 0x00
#define PACKET_TYPE_LORA 0x01

// crc types
#define CRC_OFF 0x01
#define CRC_1_BYTE 0x00
#define CRC_2_BYTE 0x02
#define CRC_1_BYTE_INV 0x04
#define CRC_2_BYTE_INV 0x06

//irq priority
#define IRQ_GPIO_PRIORITY (4)

static void *gSpiHandle;

static uint8_t pui8ReceiveBuffer[LORA_RADIO_MAX_PHYSICAL_PACKET];
static lora_radio_physical_packet_t sLoRaPhysicalPacket;

#define MAX_CALLBACK 12
static uint32_t gui32LoRaRadioCallbackListLength;
static lora_radio_irq_handler_t psLoRaRadioCallbackList[MAX_CALLBACK];

static void lora_radio_isr(void);

static void sx1262_block_on_busy(void)
{
    uint32_t state = 1;

    while (state) {
        am_hal_gpio_state_read(AM_BSP_GPIO_RADIO_BUSY, AM_HAL_GPIO_INPUT_READ,
                               &state);
    }
}

static void sx1262_write_command(uint8_t cmd, const uint8_t *data, uint8_t len)
{
    am_hal_iom_transfer_t Transaction;

    Transaction.ui32InstrLen = 1;
    Transaction.ui32Instr = cmd;
    Transaction.eDirection = AM_HAL_IOM_TX;
    Transaction.ui32NumBytes = len;
    Transaction.pui32TxBuffer = (uint32_t *)data;
    Transaction.bContinue = false;
    Transaction.ui8RepeatCount = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

    sx1262_block_on_busy();
    am_hal_iom_blocking_transfer(gSpiHandle, &Transaction);
}

static void sx1262_write_registers(uint16_t addr, const uint8_t *data,
                                   uint8_t len)
{
    uint32_t instruction = (CMD_WRITEREGISTER << 16) | addr;

    am_hal_iom_transfer_t Transaction;

    Transaction.ui32InstrLen = 3;
    Transaction.ui32Instr = instruction;
    Transaction.eDirection = AM_HAL_IOM_TX;
    Transaction.ui32NumBytes = len;
    Transaction.pui32TxBuffer = (uint32_t *)data;
    Transaction.bContinue = false;
    Transaction.ui8RepeatCount = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

    sx1262_block_on_busy();
    am_hal_iom_blocking_transfer(gSpiHandle, &Transaction);
}

static void sx1262_write_buffer(uint8_t off, const uint8_t *data, uint8_t len)
{
    am_hal_iom_transfer_t Transaction;

    Transaction.ui32InstrLen = 2;
    Transaction.ui32Instr = (CMD_WRITEBUFFER << 8) | off;
    Transaction.eDirection = AM_HAL_IOM_TX;
    Transaction.ui32NumBytes = len;
    Transaction.pui32TxBuffer = (uint32_t *)data;
    Transaction.bContinue = false;
    Transaction.ui8RepeatCount = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

    sx1262_block_on_busy();
    am_hal_iom_blocking_transfer(gSpiHandle, &Transaction);
}

static void sx1262_write_fifo(uint8_t *buf, uint8_t len)
{
    static const uint8_t ui8FifoOffsets[] = {0, 0};
    sx1262_write_command(CMD_SETBUFFERBASEADDRESS, ui8FifoOffsets, 2);

    sx1262_write_buffer(0, buf, len);
}

static uint8_t sx1262_read_command(uint8_t cmd, uint8_t *data, uint8_t len)
{
    am_hal_iom_transfer_t Transaction;

    Transaction.ui32InstrLen = 1;
    Transaction.ui32Instr = cmd;
    Transaction.eDirection = AM_HAL_IOM_RX;
    Transaction.ui32NumBytes = len;
    Transaction.pui32RxBuffer = (uint32_t *)data;
    Transaction.bContinue = false;
    Transaction.ui8RepeatCount = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

    sx1262_block_on_busy();
    am_hal_iom_blocking_transfer(gSpiHandle, &Transaction);
}

static void sx1262_read_registers(uint16_t addr, uint8_t *data, uint8_t len)
{
    uint16_t offset =
        __bswap16(addr); // offset = (addr >> 8) | ((addr & 0xFF) << 8)
    uint32_t instruction = (offset << 8) | CMD_READREGISTER;

    am_hal_iom_transfer_t Transaction;

    Transaction.ui32InstrLen = 0;
    Transaction.ui32Instr = 0;
    Transaction.eDirection = AM_HAL_IOM_TX;
    Transaction.ui32NumBytes = 4;
    Transaction.pui32TxBuffer = (uint32_t *)&instruction;
    Transaction.bContinue = true;
    Transaction.ui8RepeatCount = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

    sx1262_block_on_busy();
    am_hal_iom_blocking_transfer(gSpiHandle, &Transaction);

    Transaction.ui32InstrLen = 0;
    Transaction.ui32Instr = 0;
    Transaction.eDirection = AM_HAL_IOM_RX;
    Transaction.ui32NumBytes = len;
    Transaction.pui32RxBuffer = (uint32_t *)data;
    Transaction.bContinue = false;
    Transaction.ui8RepeatCount = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

    am_hal_iom_blocking_transfer(gSpiHandle, &Transaction);
}

static void sx1262_read_buffer(uint8_t off, uint8_t *data, uint8_t len)
{
    uint32_t instruction = (CMD_READBUFFER << 16) | (off << 8);

    am_hal_iom_transfer_t Transaction;

    Transaction.ui32InstrLen = 3;
    Transaction.ui32Instr = instruction;
    Transaction.eDirection = AM_HAL_IOM_RX;
    Transaction.ui32NumBytes = len;
    Transaction.pui32RxBuffer = (uint32_t *)data;
    Transaction.bContinue = false;
    Transaction.ui8RepeatCount = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

    sx1262_block_on_busy();
    am_hal_iom_blocking_transfer(gSpiHandle, &Transaction);
}

static uint8_t sx1262_read_fifo(uint8_t *buf)
{
    // get buffer status
    uint8_t status[4];
    sx1262_read_command(CMD_GETRXBUFFERSTATUS, status, 4);

    // read buffer
    uint8_t len = status[1];
    uint8_t off = status[2];
    sx1262_read_buffer(off, buf, len);

    // return length
    return len;
}

void sx1262_set_mode(uint8_t mode, uint32_t ui32Parameter)
{
    switch (mode) {
    case CMD_SETSLEEP:
    case CMD_SETSTANDBY:
        sx1262_write_command(mode, (uint8_t *)&ui32Parameter, 1);
        break;
    case CMD_SETFS:
    case CMD_SETTXCONTINUOUSWAVE:
        sx1262_write_command(mode, NULL, 0);
        break;
    case CMD_SETTX:
    case CMD_SETRX: {
        uint8_t timeout[3] = {(ui32Parameter >> 16) & 0xFF,
                              (ui32Parameter >> 8) & 0xFF,
                              ui32Parameter & 0xFF};
        sx1262_write_command(mode, timeout, 3);
    } break;
    }
}

static void sx1262_config_regulator()
{
    uint8_t mode = REGMODE_DCDC;
    sx1262_write_command(CMD_SETREGULATORMODE, &mode, 1);
}

// use DIO2 to drive antenna rf switch
static void sx1262_set_dio2_rf_switch_ctrl(uint8_t enable)
{
    sx1262_write_command(CMD_SETDIO2ASRFSWITCHCTRL, &enable, 1);
}

// set radio to PACKET_TYPE_LORA or PACKET_TYPE_FSK mode
static void sx1262_init_packet_type()
{
    uint8_t type = PACKET_TYPE_LORA;
    sx1262_write_command(CMD_SETPACKETTYPE, &type, 1);
}

// calibrate the image rejection
static void CalibrateImage(uint32_t freq)
{
    static const struct {
        uint32_t min;
        uint32_t max;
        uint8_t freq[2];
    } bands[] = {
        {430000000, 440000000, (uint8_t[]){0x6B, 0x6F}},
        {470000000, 510000000, (uint8_t[]){0x75, 0x81}},
        {779000000, 787000000, (uint8_t[]){0xC1, 0xC5}},
        {863000000, 870000000, (uint8_t[]){0xD7, 0xDB}},
        {902000000, 928000000, (uint8_t[]){0xE1, 0xE9}},
    };

    for (int i = 0; i < sizeof(bands) / sizeof(bands[0]); i++) {
        if (freq >= bands[i].min && freq <= bands[i].max) {
            sx1262_write_command(CMD_CALIBRATEIMAGE, bands[i].freq, 2);
        }
    }
}

static void sx1262_set_frequency(uint32_t freq)
{
    CalibrateImage(freq);

    uint32_t v = (uint32_t)(((uint64_t)freq << 25) / 32000000);
    uint32_t f = __bswap32(v);

    sx1262_write_command(CMD_SETRFFREQUENCY, (uint8_t *)&f, 4);
}

static void
sx1262_config_modulation(lora_radio_modulation_t *psModulationParameters)
{
    uint8_t param[4];
    param[0] = psModulationParameters->eSpreadingFactor;
    switch (psModulationParameters->eBandwidth) {
    case LORA_RADIO_BW_7:
        param[1] = 0x00;
        psModulationParameters->eLowDataRateOptimization = 0x01;
        break;
    case LORA_RADIO_BW_10:
        param[1] = 0x08;
        psModulationParameters->eLowDataRateOptimization = 0x01;
        break;
    case LORA_RADIO_BW_15:
        param[1] = 0x01;
        psModulationParameters->eLowDataRateOptimization = 0x01;
        break;
    case LORA_RADIO_BW_20:
        param[1] = 0x09;
        psModulationParameters->eLowDataRateOptimization = 0x01;
        break;
    case LORA_RADIO_BW_31:
        param[1] = 0x02;
        psModulationParameters->eLowDataRateOptimization = 0x01;
        break;
    case LORA_RADIO_BW_41:
        param[1] = 0x0A;
        if (psModulationParameters->eSpreadingFactor >= LORA_RADIO_SF9) {
            psModulationParameters->eLowDataRateOptimization = 0x01;
        } else {
            psModulationParameters->eLowDataRateOptimization = 0x00;
        }
        break;
    case LORA_RADIO_BW_62:
        param[1] = 0x03;
        if (psModulationParameters->eSpreadingFactor >= LORA_RADIO_SF10) {
            psModulationParameters->eLowDataRateOptimization = 0x01;
        } else {
            psModulationParameters->eLowDataRateOptimization = 0x00;
        }
        break;
    case LORA_RADIO_BW_125:
        if (psModulationParameters->eSpreadingFactor >= LORA_RADIO_SF11) {
            psModulationParameters->eLowDataRateOptimization = 0x01;
        } else {
            psModulationParameters->eLowDataRateOptimization = 0x00;
        }
        param[1] = 0x04;
        break;
    case LORA_RADIO_BW_250:
        if (psModulationParameters->eSpreadingFactor >= LORA_RADIO_SF12) {
            psModulationParameters->eLowDataRateOptimization = 0x01;
        } else {
            psModulationParameters->eLowDataRateOptimization = 0x00;
        }
        param[1] = 0x05;
        break;
    case LORA_RADIO_BW_500:
        param[1] = 0x06;
        psModulationParameters->eLowDataRateOptimization = 0x00;
        break;
    }

    param[2] = psModulationParameters->eCodingRate + 1;
    param[3] = psModulationParameters->eLowDataRateOptimization;

    sx1262_write_command(CMD_SETMODULATIONPARAMS, param, 4);
}

static void sx1262_config_packet(lora_radio_packet_t *psPacketParameters)
{
    uint8_t param[6];

    uint16_t ui16PreambleLength = psPacketParameters->ui16PreambleLength;
    param[0] = (ui16PreambleLength >> 8);
    param[1] = ui16PreambleLength;
    param[2] = psPacketParameters->ePacketLength;
    param[3] = psPacketParameters->ui8PayloadLength;
    param[4] = psPacketParameters->eCRC;
    param[5] = psPacketParameters->eIQ;

    sx1262_write_command(CMD_SETPACKETPARAMS, param, 6);
}

static void sx1262_interrupt_clear(uint16_t mask)
{
    uint8_t buf[2] = {mask >> 8, mask & 0xFF};
    sx1262_write_command(CMD_CLEARIRQSTATUS, buf, 2);
}

static void sx1262_stop_timer_on_preamble(uint8_t enable)
{
    sx1262_write_command(CMD_STOPTIMERONPREAMBLE, &enable, 1);
}

// set number of symbols for reception
static void sx1262_set_symbol_timeout(uint8_t nsym)
{
    sx1262_write_command(CMD_SETLORASYMBNUMTIMEOUT, &nsym, 1);
}

static uint16_t sx1262_interrupt_status_get(void)
{
    uint8_t buf[3];
    sx1262_read_command(CMD_GETIRQSTATUS, buf, 3);

    return ((uint16_t)(buf[1] << 8) | buf[2]);
}

static void sx1262_get_packet_status(int8_t *rssi, int8_t *snr, int8_t *rscp)
{
    uint8_t buf[3];
    sx1262_read_command(CMD_GETPACKETSTATUS, buf, 3);
    *rssi = -buf[0] / 2;
    *snr = buf[1] / 4;
    *rscp = -buf[2] / 2;
}

static uint16_t sx1262_get_error_status(void)
{
    uint8_t buf[3];
    sx1262_read_command(CMD_GETDEVICEERRORS, buf, 3);
    return ((uint16_t)(buf[1] << 8) | buf[2]);
}

static uint16_t sx1262_get_device_status(void)
{
    uint8_t buf;
    sx1262_read_command(CMD_GETSTATUS, &buf, 1);
    return buf;
}

// set and enable irq mask for dio1
static void sx1262_interrupt_enable(uint16_t mask)
{
    uint8_t param[] = {mask >> 8, mask & 0xFF, mask >> 8, mask & 0xFF,
                       0x00,      0x00,        0x00,      0x00};
    sx1262_write_command(CMD_SETDIOIRQPARAMS, param, 8);
}

// set tx power (in dBm)
static void sx1262_set_power(int8_t i8Power)
{
    // high power PA: -9 ... +22 dBm
    if (i8Power > 22) {
        i8Power = 22;
    }

    if (i8Power < -9) {
        i8Power = -9;
    }

    // set PA config (and reset OCP to 140mA)
    sx1262_write_command(CMD_SETPACONFIG,
                         (const uint8_t[]){0x04, 0x07, 0x00, 0x01}, 4);

    uint8_t ui8TransmitParameters[2];
    ui8TransmitParameters[0] = (uint8_t)i8Power;
    ui8TransmitParameters[1] = 0x04; // ramp time 200us
    sx1262_write_command(CMD_SETTXPARAMS, ui8TransmitParameters, 2);
}

// set sync word for LoRa
static void sx1262_set_syncword(uint16_t ui16SyncWord)
{
    uint8_t buf[2] = {ui16SyncWord >> 8, ui16SyncWord & 0xFF};
    sx1262_write_registers(REG_LORASYNCWORDMSB, buf, 2);
}

static void sx1262_transmit(void *pHandle, lora_radio_transfer_t *psTransaction)
{
    lora_radio_modulation_t *psModulationParameters =
        psTransaction->psModulationParameters;
    lora_radio_packet_t *psPacketParameters = psTransaction->psPacketParameters;

    sx1262_set_mode(CMD_SETSTANDBY, STDBY_RC);

    sx1262_init_packet_type();
    sx1262_config_packet(psPacketParameters);
    sx1262_config_modulation(psModulationParameters);

    sx1262_set_frequency(psTransaction->ui32Frequency);
    sx1262_set_power(psTransaction->ui8Power);
    sx1262_set_syncword(psTransaction->ui32SyncWord);

    sx1262_write_fifo(psTransaction->pui8Payload,
                      psPacketParameters->ui8PayloadLength);
    sx1262_interrupt_clear(LORA_RADIO_IRQ_ALL);
    sx1262_interrupt_enable(LORA_RADIO_TXDONE | LORA_RADIO_TIMEOUT);
    sx1262_set_mode(CMD_SETTX, (psTransaction->ui32Timeout >> 8) & 0xFFFFFF);
}

static void sx1262_transmit_carrier(void *pHandle,
                                    lora_radio_transfer_t *psTransaction)
{
    sx1262_config_regulator();
    sx1262_set_dio2_rf_switch_ctrl(1);
    sx1262_set_mode(CMD_SETSTANDBY, STDBY_RC);

    sx1262_set_frequency(psTransaction->ui32Frequency);
    sx1262_set_power(psTransaction->ui8Power);
    sx1262_interrupt_clear(LORA_RADIO_IRQ_ALL);

    sx1262_set_mode(CMD_SETTXCONTINUOUSWAVE, 0);
}

static void sx1262_receive(void *pHandle, lora_radio_transfer_t *psTransaction)
{
    lora_radio_modulation_t *psModulationParameters =
        psTransaction->psModulationParameters;
    lora_radio_packet_t *psPacketParameters = psTransaction->psPacketParameters;

    sx1262_set_mode(CMD_SETSTANDBY, STDBY_RC);

    sx1262_init_packet_type();
    sx1262_config_packet(psPacketParameters);
    sx1262_config_modulation(psModulationParameters);

    sx1262_set_frequency(psTransaction->ui32Frequency);
    sx1262_set_syncword(psTransaction->ui32SyncWord);

    // FIXME:
    // The following two lines causes immediate timeout and freezes
    // for variable length packets.
    /*
    if (psPacketParameters->ePacketLength == LORA_RADIO_PACKET_LENGTH_FIXED)
    {
        sx1262_stop_timer_on_preamble(0);
        sx1262_set_symbol_timeout(psTransaction->ui32Timeout & 0xFF);
    }
    */

    sx1262_interrupt_clear(LORA_RADIO_IRQ_ALL);
    sx1262_interrupt_enable(LORA_RADIO_RXDONE | LORA_RADIO_TIMEOUT);

    sx1262_set_mode(CMD_SETRX, (psTransaction->ui32Timeout >> 8) & 0xFFFFFF);
}

uint32_t lora_radio_initialize(void **ppHandle)
{
    uint32_t status = AM_HAL_STATUS_SUCCESS;

    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_NRESET, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(AM_BSP_GPIO_RADIO_NRESET,
                            AM_HAL_GPIO_OUTPUT_TRISTATE_DISABLE);
    am_hal_gpio_state_write(AM_BSP_GPIO_RADIO_NRESET, AM_HAL_GPIO_OUTPUT_SET);

    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_BUSY, g_AM_HAL_GPIO_INPUT);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_DIO1, g_AM_HAL_GPIO_INPUT);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_DIO3, g_AM_HAL_GPIO_INPUT);

    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_CLK, g_AM_BSP_GPIO_RADIO_CLK);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_MISO, g_AM_BSP_GPIO_RADIO_MISO);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_MOSI, g_AM_BSP_GPIO_RADIO_MOSI);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_NSS, g_AM_BSP_GPIO_RADIO_NSS);

    am_hal_gpio_interrupt_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_RADIO_DIO1));
    am_hal_gpio_interrupt_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_RADIO_DIO3));

    am_hal_gpio_interrupt_register(AM_BSP_GPIO_RADIO_DIO1, lora_radio_isr);
    am_hal_gpio_interrupt_register(AM_BSP_GPIO_RADIO_DIO3, lora_radio_isr);

    am_hal_iom_config_t sSpiConfig;
    sSpiConfig.eInterfaceMode = AM_HAL_IOM_SPI_MODE;
    sSpiConfig.ui32ClockFreq = AM_HAL_IOM_4MHZ;
    sSpiConfig.eSpiMode = AM_HAL_IOM_SPI_MODE_0;

    if (am_hal_iom_initialize(3, &gSpiHandle) != AM_HAL_STATUS_SUCCESS) {
        return LORA_RADIO_STATUS_FAIL;
    }

    if (am_hal_iom_power_ctrl(gSpiHandle, AM_HAL_SYSCTRL_WAKE, false) !=
        AM_HAL_STATUS_SUCCESS) {
        return LORA_RADIO_STATUS_FAIL;
    }

    if (am_hal_iom_configure(gSpiHandle, &sSpiConfig) !=
        AM_HAL_STATUS_SUCCESS) {
        return LORA_RADIO_STATUS_FAIL;
    }

    if (am_hal_iom_enable(gSpiHandle) != AM_HAL_STATUS_SUCCESS) {
        return LORA_RADIO_STATUS_FAIL;
    }

    lora_radio_reset(&ppHandle);

    //    status = sx1262_get_device_status();

    am_hal_gpio_interrupt_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_RADIO_DIO1));
    am_hal_gpio_interrupt_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_RADIO_DIO3));
    NVIC_EnableIRQ(GPIO_IRQn);

    //set priority below 0 to facilitate *FromISR FreeRTOS APIs
    NVIC_SetPriority(GPIO_IRQn, IRQ_GPIO_PRIORITY);

    sx1262_set_mode(CMD_SETSTANDBY, STDBY_RC);
    sx1262_config_regulator();
    sx1262_set_dio2_rf_switch_ctrl(1);
    sx1262_init_packet_type();

    return LORA_RADIO_STATUS_SUCCESS;
}

uint32_t lora_radio_deinitialize(void *pHandle)
{
    am_hal_gpio_interrupt_disable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_RADIO_DIO1));
    am_hal_gpio_interrupt_disable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_RADIO_DIO3));

    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_NRESET, g_AM_HAL_GPIO_DISABLE);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_BUSY, g_AM_HAL_GPIO_DISABLE);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_DIO1, g_AM_HAL_GPIO_DISABLE);
    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_DIO3, g_AM_HAL_GPIO_DISABLE);

    am_hal_iom_uninitialize(gSpiHandle);
    am_hal_iom_power_ctrl(gSpiHandle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
    am_bsp_iom_pins_disable(3, AM_HAL_IOM_SPI_MODE);
    am_hal_iom_disable(gSpiHandle);

    return LORA_RADIO_STATUS_SUCCESS;
}

uint32_t lora_radio_reset(void *pHandle)
{
    am_hal_gpio_state_write(AM_BSP_GPIO_RADIO_NRESET, AM_HAL_GPIO_OUTPUT_CLEAR);
    am_util_delay_us(100);
    am_hal_gpio_state_write(AM_BSP_GPIO_RADIO_NRESET, AM_HAL_GPIO_OUTPUT_SET);

    sx1262_config_regulator();
    sx1262_set_mode(CMD_SETSTANDBY, STDBY_RC);
    sx1262_set_dio2_rf_switch_ctrl(1);
    sx1262_init_packet_type();

    return LORA_RADIO_STATUS_SUCCESS;
}

uint32_t lora_radio_power_ctrl(void *pHandle,
                               lora_radio_power_state_e ePowerState)
{
    switch (ePowerState) {
    case LORA_RADIO_SLEEP:
        sx1262_set_mode(CMD_SETSLEEP, SLEEP_WARM);
        break;
    case LORA_RADIO_DEEPSLEEP:
        sx1262_set_mode(CMD_SETSLEEP, SLEEP_COLD);
        break;
    case LORA_RADIO_STANDBY:
        sx1262_set_mode(CMD_SETSTANDBY, STDBY_RC);
        break;
    default:
        return LORA_RADIO_STATUS_INVALID_ARG;
    }

    return LORA_RADIO_STATUS_SUCCESS;
}

uint32_t lora_radio_transfer(void *pHandle,
                             lora_radio_transfer_t *psTransaction)
{
    switch (psTransaction->eMode) {
    case LORA_RADIO_TX:
        sx1262_transmit(pHandle, psTransaction);
        break;
    case LORA_RADIO_RX:
        sx1262_receive(pHandle, psTransaction);
        break;
    case LORA_RADIO_TXCARRIER:
        sx1262_transmit_carrier(pHandle, psTransaction);
        break;
    default:
        return LORA_RADIO_STATUS_INVALID_ARG;
    }

    return LORA_RADIO_STATUS_SUCCESS;
}

void lora_radio_callback_list_init()
{
    memset(psLoRaRadioCallbackList, 0,
           MAX_CALLBACK * sizeof(lora_radio_irq_handler_t));
    gui32LoRaRadioCallbackListLength = 0;
}

void lora_radio_callback_list_deinit()
{
    memset(psLoRaRadioCallbackList, 0,
           MAX_CALLBACK * sizeof(lora_radio_irq_handler_t));
    gui32LoRaRadioCallbackListLength = 0;
}

uint32_t lora_radio_callback_register(lora_radio_irq_e eIrq,
                                      lora_radio_callback_t pfnCallback)
{
    if (gui32LoRaRadioCallbackListLength >= MAX_CALLBACK) {
        return LORA_RADIO_STATUS_FAIL;
    }
    psLoRaRadioCallbackList[gui32LoRaRadioCallbackListLength].eIRQ = eIrq;
    psLoRaRadioCallbackList[gui32LoRaRadioCallbackListLength].pfnCallback =
        pfnCallback;
    gui32LoRaRadioCallbackListLength++;

    return LORA_RADIO_STATUS_SUCCESS;
}

uint32_t lora_radio_callback_deregister(lora_radio_irq_e eIrq,
                                        lora_radio_callback_t pfnCallback)
{
    uint8_t i = 0;
    uint8_t ui8Found = 0;

    for (i = 0; i < gui32LoRaRadioCallbackListLength; i++) {
        if ((psLoRaRadioCallbackList[i].eIRQ == eIrq) &&
            (psLoRaRadioCallbackList[i].pfnCallback == pfnCallback)) {
            ui8Found = 1;
            break;
        }
    }

    if (ui8Found) {
        while (i < (gui32LoRaRadioCallbackListLength - 1)) {
            psLoRaRadioCallbackList[i].eIRQ =
                psLoRaRadioCallbackList[i + 1].eIRQ;
            psLoRaRadioCallbackList[i].pfnCallback =
                psLoRaRadioCallbackList[i + 1].pfnCallback;
            i++;
        }
        gui32LoRaRadioCallbackListLength--;

        return LORA_RADIO_STATUS_SUCCESS;
    }

    return LORA_RADIO_STATUS_FAIL;
}

static void lora_radio_isr(void)
{
    uint8_t i, ui8PacketLength;
    uint16_t ui16IrqStatus = sx1262_interrupt_status_get();

    if (ui16IrqStatus & LORA_RADIO_RXDONE) {
        ui8PacketLength = sx1262_read_fifo(pui8ReceiveBuffer);
        sLoRaPhysicalPacket.pui8Payload = pui8ReceiveBuffer;
        sLoRaPhysicalPacket.ui8PayloadLength = ui8PacketLength;

        sx1262_get_packet_status(&sLoRaPhysicalPacket.i8Rssi,
                                 &sLoRaPhysicalPacket.i8Snr,
                                 &sLoRaPhysicalPacket.i8Rscp);
    }

    for (i = 0; i < gui32LoRaRadioCallbackListLength; i++) {
        if ((psLoRaRadioCallbackList[i].eIRQ & ui16IrqStatus) &&
            (ui16IrqStatus & LORA_RADIO_RXDONE)) {
            // callback will still trigger if ui8PacketLength is zero
            // if ((ui8PacketLength > 0) && (psLoRaRadioCallbackList[i].pfnCallback))
            if (psLoRaRadioCallbackList[i].pfnCallback)
                psLoRaRadioCallbackList[i].pfnCallback(&sLoRaPhysicalPacket);
        } else if (psLoRaRadioCallbackList[i].eIRQ & ui16IrqStatus) {
            if (psLoRaRadioCallbackList[i].pfnCallback)
                psLoRaRadioCallbackList[i].pfnCallback(NULL);
        }
    }

    sx1262_interrupt_enable(0);
    sx1262_interrupt_clear(LORA_RADIO_IRQ_ALL);
}
