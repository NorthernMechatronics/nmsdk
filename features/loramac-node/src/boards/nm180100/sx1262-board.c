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
#include <stdlib.h>

#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOSConfig.h>

#include <radio.h>
#include <sx126x-board.h>
#include <utilities.h>

#define SX1262_IOM_MODULE 3
#define RADIO_NRESET      44
#define RADIO_BUSY        39
#define RADIO_DIO1        40
#define RADIO_DIO3        47

#define RADIO_MISO     43
#define RADIO_MOSI     38
#define RADIO_CLK      42
#define RADIO_NSS      36
#define RADIO_NSS_CHNL 1

static const am_hal_gpio_pincfg_t s_RADIO_DIO1 = {
    .uFuncSel       = AM_HAL_PIN_40_GPIO,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eGPInput       = AM_HAL_GPIO_PIN_INPUT_ENABLE,
    .eIntDir        = AM_HAL_GPIO_PIN_INTDIR_LO2HI};

static const am_hal_gpio_pincfg_t s_RADIO_CLK = {
    .uFuncSel       = AM_HAL_PIN_42_M3SCK,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .uIOMnum        = 3};

static const am_hal_gpio_pincfg_t s_RADIO_MISO = {
    .uFuncSel = AM_HAL_PIN_43_M3MISO, .uIOMnum = 3};

static const am_hal_gpio_pincfg_t s_RADIO_MOSI = {
    .uFuncSel       = AM_HAL_PIN_38_M3MOSI,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .uIOMnum        = 3};

static const am_hal_gpio_pincfg_t s_RADIO_NSS = {
    .uFuncSel       = AM_HAL_PIN_36_NCE36,
    .eDriveStrength = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eGPOutcfg      = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput       = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir        = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .uIOMnum        = 3,
    .uNCE           = 1,
    .eCEpol         = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW};

static am_hal_iom_config_t SX126xSpi;
static void *              SX126xHandle;

static RadioOperatingModes_t OperatingMode;

static uint8_t SX126xSpiRead(uint8_t cmd, uint8_t *buf, uint8_t len)
{
    am_hal_iom_transfer_t rx;

    uint8_t status;

    rx.ui32InstrLen = 1;
    rx.ui32Instr = cmd;
    rx.eDirection = AM_HAL_IOM_RX;
    rx.ui32NumBytes = 1;
    rx.pui32RxBuffer = (uint32_t *)&status;
    rx.bContinue = true;
    rx.ui8RepeatCount = 0;
    rx.ui32PauseCondition = 0;
    rx.ui32StatusSetClr = 0;
    rx.uPeerInfo.ui32SpiChipSelect = RADIO_NSS_CHNL;
    am_hal_iom_blocking_transfer(SX126xHandle, &rx);

    rx.ui32InstrLen = 0;
    rx.ui32Instr = 0;
    rx.eDirection = AM_HAL_IOM_RX;
    rx.ui32NumBytes = len;
    rx.pui32RxBuffer = (uint32_t *)buf;
    rx.bContinue = false;
    rx.ui8RepeatCount = 0;
    rx.ui32PauseCondition = 0;
    rx.ui32StatusSetClr = 0;
    rx.uPeerInfo.ui32SpiChipSelect = RADIO_NSS_CHNL;
    am_hal_iom_blocking_transfer(SX126xHandle, &rx);

    return status;
}

static void SX126xSpiReadRegisters(uint16_t addr, uint8_t *buf, uint8_t len)
{
    uint32_t inst = (__builtin_bswap16(addr) << 8) | RADIO_READ_REGISTER;
    am_hal_iom_transfer_t tx, rx;

    tx.ui32InstrLen                = 0;
    tx.ui32Instr                   = 0;
    tx.eDirection                  = AM_HAL_IOM_TX;
    tx.ui32NumBytes                = 4;
    tx.pui32TxBuffer               = (uint32_t *)&inst;
    tx.bContinue                   = true;
    tx.ui8RepeatCount              = 0;
    tx.ui32PauseCondition          = 0;
    tx.ui32StatusSetClr            = 0;
    tx.uPeerInfo.ui32SpiChipSelect = RADIO_NSS_CHNL;
    am_hal_iom_blocking_transfer(SX126xHandle, &tx);

    rx.ui32InstrLen                = 0;
    rx.ui32Instr                   = 0;
    rx.eDirection                  = AM_HAL_IOM_RX;
    rx.ui32NumBytes                = len;
    rx.pui32RxBuffer               = (uint32_t *)buf;
    rx.bContinue                   = false;
    rx.ui8RepeatCount              = 0;
    rx.ui32PauseCondition          = 0;
    rx.ui32StatusSetClr            = 0;
    rx.uPeerInfo.ui32SpiChipSelect = RADIO_NSS_CHNL;
    am_hal_iom_blocking_transfer(SX126xHandle, &rx);
}

static void SX126xSpiReadBuffer(uint8_t ofs, uint8_t *buf, uint8_t len)
{
    am_hal_iom_transfer_t rx;
    uint32_t              inst = (RADIO_READ_BUFFER << 16) | (ofs << 8);

    rx.ui32InstrLen                = 3;
    rx.ui32Instr                   = inst;
    rx.eDirection                  = AM_HAL_IOM_RX;
    rx.ui32NumBytes                = len;
    rx.pui32RxBuffer               = (uint32_t *)buf;
    rx.bContinue                   = false;
    rx.ui8RepeatCount              = 0;
    rx.ui32PauseCondition          = 0;
    rx.ui32StatusSetClr            = 0;
    rx.uPeerInfo.ui32SpiChipSelect = RADIO_NSS_CHNL;

    am_hal_iom_blocking_transfer(SX126xHandle, &rx);
}

static void SX126xSpiWrite(uint8_t cmd, uint8_t *buf, uint8_t len)
{
    am_hal_iom_transfer_t tx;

    tx.ui32InstrLen                = 1;
    tx.ui32Instr                   = cmd;
    tx.eDirection                  = AM_HAL_IOM_TX;
    tx.ui32NumBytes                = len;
    tx.pui32TxBuffer               = (uint32_t *)buf;
    tx.bContinue                   = false;
    tx.ui8RepeatCount              = 0;
    tx.ui32PauseCondition          = 0;
    tx.ui32StatusSetClr            = 0;
    tx.uPeerInfo.ui32SpiChipSelect = RADIO_NSS_CHNL;

    am_hal_iom_blocking_transfer(SX126xHandle, &tx);
}

static void SX126xSpiWriteRegisters(uint16_t addr, const uint8_t *buf,
                                    uint8_t len)
{
    am_hal_iom_transfer_t tx;
    uint32_t              inst = (RADIO_WRITE_REGISTER << 16) | addr;

    tx.ui32InstrLen                = 3;
    tx.ui32Instr                   = inst;
    tx.eDirection                  = AM_HAL_IOM_TX;
    tx.ui32NumBytes                = len;
    tx.pui32TxBuffer               = (uint32_t *)buf;
    tx.bContinue                   = false;
    tx.ui8RepeatCount              = 0;
    tx.ui32PauseCondition          = 0;
    tx.ui32StatusSetClr            = 0;
    tx.uPeerInfo.ui32SpiChipSelect = RADIO_NSS_CHNL;

    am_hal_iom_blocking_transfer(SX126xHandle, &tx);
}

static void SX126xSpiWriteBuffer(uint8_t ofs, const uint8_t *buf, uint8_t len)
{
    am_hal_iom_transfer_t tx;
    uint32_t              inst = (RADIO_WRITE_BUFFER << 8) | ofs;

    tx.ui32InstrLen                = 2;
    tx.ui32Instr                   = inst;
    tx.eDirection                  = AM_HAL_IOM_TX;
    tx.ui32NumBytes                = len;
    tx.pui32TxBuffer               = (uint32_t *)buf;
    tx.bContinue                   = false;
    tx.ui8RepeatCount              = 0;
    tx.ui32PauseCondition          = 0;
    tx.ui32StatusSetClr            = 0;
    tx.uPeerInfo.ui32SpiChipSelect = RADIO_NSS_CHNL;

    am_hal_iom_blocking_transfer(SX126xHandle, &tx);
}

void SX126xIoInit(void)
{
    am_hal_gpio_pinconfig(RADIO_NRESET, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_pinconfig(RADIO_BUSY, g_AM_HAL_GPIO_INPUT);
    am_hal_gpio_pinconfig(RADIO_DIO1, s_RADIO_DIO1);

    am_hal_gpio_pinconfig(RADIO_CLK, s_RADIO_CLK);
    am_hal_gpio_pinconfig(RADIO_MISO, s_RADIO_MISO);
    am_hal_gpio_pinconfig(RADIO_MOSI, s_RADIO_MOSI);
    am_hal_gpio_pinconfig(RADIO_NSS, s_RADIO_NSS);

    SX126xSpi.eInterfaceMode = AM_HAL_IOM_SPI_MODE;
    SX126xSpi.ui32ClockFreq  = AM_HAL_IOM_4MHZ;
    SX126xSpi.eSpiMode       = AM_HAL_IOM_SPI_MODE_0;

    am_hal_iom_initialize(SX1262_IOM_MODULE, &SX126xHandle);
    am_hal_iom_power_ctrl(SX126xHandle, AM_HAL_SYSCTRL_WAKE, false);
    am_hal_iom_configure(SX126xHandle, &SX126xSpi);
    am_hal_iom_enable(SX126xHandle);
}

void SX126xIoIrqInit(DioIrqHandler dioIrq)
{
    am_hal_gpio_interrupt_register(RADIO_DIO1, (am_hal_gpio_handler_t)dioIrq);
    am_hal_gpio_interrupt_clear(AM_HAL_GPIO_BIT(RADIO_DIO1));
    am_hal_gpio_interrupt_enable(AM_HAL_GPIO_BIT(RADIO_DIO1));
    NVIC_EnableIRQ(GPIO_IRQn);
}

void SX126xIoDeInit(void)
{
    am_hal_gpio_interrupt_disable(AM_HAL_GPIO_BIT(RADIO_DIO1));
    am_hal_gpio_interrupt_clear(AM_HAL_GPIO_BIT(RADIO_DIO1));

    am_hal_iom_disable(SX126xHandle);
    am_hal_iom_power_ctrl(SX126xHandle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
    am_hal_iom_uninitialize(SX126xHandle);

    am_hal_gpio_pinconfig(RADIO_NRESET, g_AM_HAL_GPIO_DISABLE);
    am_hal_gpio_pinconfig(RADIO_BUSY, g_AM_HAL_GPIO_DISABLE);
    am_hal_gpio_pinconfig(RADIO_DIO1, g_AM_HAL_GPIO_DISABLE);

    am_hal_gpio_pinconfig(RADIO_CLK, g_AM_HAL_GPIO_DISABLE);
    am_hal_gpio_pinconfig(RADIO_MISO, g_AM_HAL_GPIO_DISABLE);
    am_hal_gpio_pinconfig(RADIO_MOSI, g_AM_HAL_GPIO_DISABLE);
    am_hal_gpio_pinconfig(RADIO_NSS, g_AM_HAL_GPIO_DISABLE);
}

void SX126xIoTcxoInit(void) {}

uint32_t SX126xGetBoardTcxoWakeupTime(void) { return 0; }

void SX126xIoRfSwitchInit(void) { SX126xSetDio2AsRfSwitchCtrl(true); }

RadioOperatingModes_t SX126xGetOperatingMode(void) { return OperatingMode; }

void SX126xSetOperatingMode(RadioOperatingModes_t mode)
{
    OperatingMode = mode;
}

void SX126xReset(void)
{
    am_hal_gpio_state_write(RADIO_NRESET, AM_HAL_GPIO_OUTPUT_CLEAR);
    am_util_delay_us(100);
    am_hal_gpio_state_write(RADIO_NRESET, AM_HAL_GPIO_OUTPUT_SET);
    am_util_delay_us(100);
}

void SX126xWaitOnBusy(void)
{
    uint32_t busy = 1;

    while (busy) {
        am_hal_gpio_state_read(RADIO_BUSY, AM_HAL_GPIO_INPUT_READ, &busy);
    }
}

void SX126xWakeup(void)
{
    CRITICAL_SECTION_BEGIN();

    uint8_t status = 0;
    SX126xSpiWrite(RADIO_GET_STATUS, &status, 1);

    SX126xWaitOnBusy();
    SX126xSetOperatingMode(MODE_STDBY_RC);
    CRITICAL_SECTION_END();
}

void SX126xWriteCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size)
{
    SX126xCheckDeviceReady();

    SX126xSpiWrite(command, buffer, size);

    if (command != RADIO_SET_SLEEP) {
        SX126xWaitOnBusy();
    }
}

uint8_t SX126xReadCommand(RadioCommands_t command, uint8_t *buffer,
                          uint16_t size)
{
    uint8_t status;

    SX126xCheckDeviceReady();
    status = SX126xSpiRead(command, buffer, size);
    SX126xWaitOnBusy();

    return status;
}

void SX126xWriteRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
    SX126xCheckDeviceReady();
    SX126xSpiWriteRegisters(address, buffer, size);
    SX126xWaitOnBusy();
}

void SX126xWriteRegister(uint16_t address, uint8_t value)
{
    SX126xWriteRegisters(address, &value, 1);
}

void SX126xReadRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
    SX126xCheckDeviceReady();
    SX126xSpiReadRegisters(address, buffer, size);
    SX126xWaitOnBusy();
}

uint8_t SX126xReadRegister(uint16_t address)
{
    uint8_t data;

    SX126xReadRegisters(address, &data, 1);
    return data;
}

void SX126xWriteBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
    SX126xCheckDeviceReady();
    SX126xSpiWriteBuffer(offset, buffer, size);
    SX126xWaitOnBusy();
}

void SX126xReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
    SX126xCheckDeviceReady();
    SX126xSpiReadBuffer(offset, buffer, size);
    SX126xWaitOnBusy();
}

void SX126xSetRfTxPower(int8_t power)
{
    SX126xSetTxParams(power, RADIO_RAMP_40_US);
}

uint8_t SX126xGetDeviceId(void) { return SX1262; }

void SX126xAntSwOn(void) {}

void SX126xAntSwOff(void) {}

bool SX126xCheckRfFrequency(uint32_t frequency) { return true; }
