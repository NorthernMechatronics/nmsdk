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
#include <string.h>

#include <am_mcu_apollo.h>
#include <am_util.h>

#include <FreeRTOS.h>
#include <FreeRTOS_CLI.h>
#include <queue.h>
#include <timers.h>

#include <rtc-board.h>
#include <systime.h>
#include <timer.h>

// MCU wake up time in ticks
#define MIN_ALARM_DELAY     (3)

#define CLOCK_PERIOD    32768
#define CLOCK_SHIFT     15
#define CLOCK_MS_MASK   0x7FFF
#define TICKS_IN_MS     (CLOCK_PERIOD * 1e-3)
#define CLOCK_SOURCE    AM_HAL_STIMER_XTAL_32KHZ

static bool    RtcInitialized           = false;
static bool    McuWakeUpTimeInitialized = false;
static int16_t McuWakeUpTimeCal         = 0;

typedef struct {
    bool     Running;
    uint64_t Ref_Ticks;
    uint64_t Alarm_Ticks;
} RtcTimerContext_t;

static RtcTimerContext_t RtcTimerContext;

static uint32_t RtcBackupRegisters[] = {0, 0};

void
am_stimer_cmpr0_isr(void)
{
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREA);

    if (RtcTimerContext.Running)
    {
        if (am_hal_stimer_counter_get() >= RtcTimerContext.Alarm_Ticks)
        {
            RtcTimerContext.Running = false;
            TimerIrqHandler();
        }
    }
}

void RtcInit(void)
{
    if (RtcInitialized == false) {
        am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREA);
        NVIC_EnableIRQ(STIMER_CMPR0_IRQn);

        am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR | AM_HAL_STIMER_CFG_FREEZE);
        am_hal_stimer_config(CLOCK_SOURCE |
                             AM_HAL_STIMER_CFG_COMPARE_A_ENABLE);

        RtcSetTimerContext();
        RtcInitialized = true;
    }
}

uint32_t RtcGetMinimumTimeout(void)
{
    return MIN_ALARM_DELAY;
}

uint32_t RtcMs2Tick(uint32_t milliseconds)
{
    return (uint32_t)(milliseconds);
}

uint32_t RtcTick2Ms(uint32_t tick)
{
    return (uint32_t)((tick >> CLOCK_SHIFT) * 1000);
}

void RtcDelayMs(uint32_t delay)
{
    am_util_delay_ms(delay);
}

uint32_t RtcSetTimerContext(void)
{
    RtcTimerContext.Ref_Ticks = am_hal_stimer_counter_get();

    return (uint32_t)RtcTimerContext.Ref_Ticks;
}

uint32_t RtcGetTimerContext(void) { return RtcTimerContext.Ref_Ticks; }

void RtcSetAlarm(uint32_t timeout) { RtcStartAlarm(timeout); }

void RtcStopAlarm(void)
{
    RtcTimerContext.Running = false;
}

void RtcStartAlarm(uint32_t timeout)
{
    RtcStopAlarm();

    RtcTimerContext.Alarm_Ticks = am_hal_stimer_counter_get() + timeout;
    RtcTimerContext.Running = true;
    am_hal_stimer_compare_delta_set(0, timeout * TICKS_IN_MS);
}

uint32_t RtcGetTimerValue(void)
{
    return (uint32_t)am_hal_stimer_counter_get();
}

uint32_t RtcGetTimerElapsedTime(void)
{
    uint64_t current = am_hal_stimer_counter_get();
    return (uint32_t)(current - RtcTimerContext.Ref_Ticks);
}

void RtcSetMcuWakeUpTime(void)
{
    if (!McuWakeUpTimeInitialized) {
    }
}

int16_t RtcGetMcuWakeUpTime(void) { return McuWakeUpTimeCal; }

uint32_t RtcGetCalendarTime(uint16_t *milliseconds)
{
    uint64_t value   = am_hal_stimer_counter_get();
    uint32_t seconds = (value >> CLOCK_SHIFT);

    uint32_t ticks_remainder = value & CLOCK_MS_MASK;
    *milliseconds            = RtcTick2Ms(ticks_remainder);

    return seconds;
}

void RtcBkupWrite(uint32_t data0, uint32_t data1)
{
    RtcBackupRegisters[0] = data0;
    RtcBackupRegisters[1] = data1;
}

void RtcBkupRead(uint32_t *data0, uint32_t *data1)
{
    *data0 = RtcBackupRegisters[0];
    *data1 = RtcBackupRegisters[1];
}

void RtcProcess(void)
{

}

TimerTime_t RtcTempCompensation(TimerTime_t period, float temperature)
{
    return (TimerTime_t)period;
}
