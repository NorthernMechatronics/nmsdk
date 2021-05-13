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

#include <am_mcu_apollo.h>
#include <am_util.h>

#include "board.h"
#include "eeprom_emulation.h"
#include "eeprom_emulation_conf.h"
#include "rtc-board.h"
#include <sx126x-board.h>

void BoardCriticalSectionBegin(uint32_t *mask)
{
    *mask = am_hal_interrupt_master_disable();
}

void BoardCriticalSectionEnd(uint32_t *mask)
{
    am_hal_interrupt_master_set(*mask);
}

void BoardInitPeriph(void)
{
    RtcInit();
    if (!eeprom_init(EEPROM_EMULATION_FLASH_PAGES)) {
        eeprom_format(EEPROM_EMULATION_FLASH_PAGES);
    }
}

void BoardInitMcu(void) { SX126xIoInit(); }

void BoardResetMcu(void)
{
    CRITICAL_SECTION_BEGIN();
    NVIC_SystemReset();
}

void BoardDeInitMcu(void) { SX126xIoDeInit(); }

uint32_t BoardGetRandomSeed(void)
{
    am_util_id_t i;
    uint32_t     ret;

    am_util_id_device(&i);
    ret = ~(i.sMcuCtrlDevice.ui32ChipID0);
    ret ^= i.sMcuCtrlDevice.ui32ChipID1;
    ret ^= i.sMcuCtrlDevice.ui32ChipRev << 3;
    ret ^= i.sMcuCtrlDevice.ui32VendorID >> 3;
    ret ^= i.sMcuCtrlDevice.ui32SKU << 6;
    ret ^= i.sMcuCtrlDevice.ui32Qualified >> 6;

    return ret;
}

uint16_t BoardBatteryMeasureVolage(void) { return 0; }

uint32_t BoardGetBatteryVoltage(void) { return 0; }

uint8_t BoardGetBatteryLevel(void) { return 0; }

uint8_t GetBoardPowerSource(void) { return USB_POWER; }

void LpmEnterStopMode(void) {}

void LpmExitStopMode(void) {}

void LpmEnterSleepMode(void) {}

void BoardLowPowerHandler(void) {}
