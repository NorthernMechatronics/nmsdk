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
#include <math.h>
#include <stdbool.h>
#include <string.h>

#include <am_mcu_apollo.h>
#include <am_util.h>

#include <rtc-board.h>
#include <systime.h>
#include <timer.h>

// The typical transition time from deep-sleep to run mode is 25us (Chapter 22.4).
// A single alarm tick using a 32.768kHz crystal is about 30.5us.  At the nominal
// processor clock speed (48MHz), 1525 instructions can be processed in one alarm
// tick.  A delay of three alarm ticks (about 96us) will be sufficient to wake-up
// and restore context.
#define MIN_ALARM_DELAY (3)

#define CLOCK_PERIOD  32768
#define CLOCK_SHIFT   15
#define CLOCK_MS_MASK 0x7FFF
#define TICKS_IN_MS   (CLOCK_PERIOD * 1e-3)
#define CLOCK_SOURCE  AM_HAL_STIMER_XTAL_32KHZ

static bool    RtcInitialized           = false;
static bool    McuWakeUpTimeInitialized = false;
static int16_t McuWakeUpTimeCal         = 0;

typedef struct {
    bool     Running;
    uint32_t Ref_Ticks;
    uint32_t Alarm_Ticks;
} RtcTimerContext_t;

static RtcTimerContext_t RtcTimerContext;
static uint32_t rtc_backup[2];

void am_stimer_cmpr0_isr(void)
{
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREA);

    if (RtcTimerContext.Running) {
        if (am_hal_stimer_counter_get() >= RtcTimerContext.Alarm_Ticks) {
            RtcTimerContext.Running = false;
            TimerIrqHandler();
        }
    }
}

void am_stimer_cmpr1_isr(void)
{
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREB);

    if (RtcTimerContext.Running) {
        if (am_hal_stimer_counter_get() >= RtcTimerContext.Alarm_Ticks) {
            RtcTimerContext.Running = false;
            TimerIrqHandler();
        }
    }
}

void RtcInit(void)
{
    if (RtcInitialized == false) {
        am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREA | AM_HAL_STIMER_INT_COMPAREB);
        NVIC_EnableIRQ(STIMER_CMPR0_IRQn);
        NVIC_EnableIRQ(STIMER_CMPR1_IRQn);

        am_hal_stimer_config(AM_HAL_STIMER_CFG_CLEAR |
                             AM_HAL_STIMER_CFG_FREEZE);
        am_hal_stimer_config(CLOCK_SOURCE |
                AM_HAL_STIMER_CFG_COMPARE_A_ENABLE |
                AM_HAL_STIMER_CFG_COMPARE_B_ENABLE);

        RtcSetTimerContext();

        am_hal_rtc_time_t hal_rtc_time;

        am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);
        am_hal_rtc_osc_select(AM_HAL_RTC_OSC_XT);
        am_hal_rtc_osc_enable();

        hal_rtc_time.ui32Hour       = 0; // 0 to 23
        hal_rtc_time.ui32Minute     = 0; // 0 to 59
        hal_rtc_time.ui32Second     = 0; // 0 to 59
        hal_rtc_time.ui32Hundredths = 00;

        hal_rtc_time.ui32DayOfMonth = 1; // 1 to 31
        hal_rtc_time.ui32Month      = 0; // 0 to 11
        hal_rtc_time.ui32Year       = 0; // years since 2000
        hal_rtc_time.ui32Century    = 0;

        am_hal_rtc_time_set(&hal_rtc_time);

        RtcInitialized = true;
    }
}

uint32_t RtcGetMinimumTimeout(void) { return MIN_ALARM_DELAY; }

uint32_t RtcMs2Tick(uint32_t milliseconds)
{
    return (uint32_t)(milliseconds * TICKS_IN_MS);
}

uint32_t RtcTick2Ms(uint32_t tick) { return ((tick >> CLOCK_SHIFT) * 1000); }

void RtcDelayMs(uint32_t delay) { am_util_delay_ms(delay); }

uint32_t RtcSetTimerContext(void)
{
    RtcTimerContext.Ref_Ticks = am_hal_stimer_counter_get();

    return RtcTimerContext.Ref_Ticks;
}

uint32_t RtcGetTimerContext(void) { return RtcTimerContext.Ref_Ticks; }

void RtcSetAlarm(uint32_t timeout) { RtcStartAlarm(timeout); }

void RtcStopAlarm(void)
{
    am_hal_stimer_int_disable(AM_HAL_STIMER_INT_COMPAREA);
    am_hal_stimer_int_disable(AM_HAL_STIMER_INT_COMPAREB);
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREA);
    am_hal_stimer_int_clear(AM_HAL_STIMER_INT_COMPAREB);
    RtcTimerContext.Running = false;
}

void RtcStartAlarm(uint32_t timeout)
{
    RtcStopAlarm();

    // timeout is already in ticks
    RtcTimerContext.Alarm_Ticks = timeout;

    uint32_t relative = timeout - RtcGetTimerElapsedTime();

    RtcTimerContext.Running     = true;
    am_hal_stimer_compare_delta_set(0, relative);
    am_hal_stimer_compare_delta_set(0, relative + 1);
    am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREA);
    am_hal_stimer_int_enable(AM_HAL_STIMER_INT_COMPAREB);
}

uint32_t RtcGetTimerValue(void) { return am_hal_stimer_counter_get(); }

uint32_t RtcGetTimerElapsedTime(void)
{
    uint32_t current = am_hal_stimer_counter_get();
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
    *milliseconds            = (ticks_remainder >> CLOCK_SHIFT) * 1000;

    return seconds;
}

void RtcBkupWrite(uint32_t data0, uint32_t data1)
{
    /*
    am_hal_stimer_nvram_set(0, data0);
    am_hal_stimer_nvram_set(1, data1);
    */
    rtc_backup[0] = data0;
    rtc_backup[1] = data1;
}

void RtcBkupRead(uint32_t *data0, uint32_t *data1)
{
    /*
    *data0 = am_hal_stimer_nvram_get(0);
    *data1 = am_hal_stimer_nvram_get(1);
    */
    *data0 = rtc_backup[0];
    *data1 = rtc_backup[1];
}

void RtcProcess(void) {}

TimerTime_t RtcTempCompensation(TimerTime_t period, float temperature)
{
    // The drift equation of a tuning fork crystal over temperature is
    //
    //   delta_f / f_0 = k * (T - T_0) ^ 2
    //
    // As the crystal is external to the module, typical values range
    // from 0.03 to 0.04.  For the NKG crystal used in the NM180100EVB
    // and the NM180310 feather board, k is -0.034 nominal and T_0 is
    // at 25C nominal.
    float T_0 = 25.0f;
    float k   = -0.034f;
    float dev = 0.0f;

    dev = k * (temperature - T_0) * (temperature - T_0);

    // for component accuracy in the ppb range, change
    // the divisor to 1.0e9f
    float correction = ((float)period * dev) / 1.0e6f;

    // This would never happen practically, but for the sake of
    // mathematical correctness we should use a signed integer to
    // accommodate negative value correction
    int32_t corrected_period = (int32_t)period + round(correction);

    // re-cast the value back to uint32_t (TimerTime_t)
    return (TimerTime_t)corrected_period;
}
