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

#include <rtc-board.h>
#include <systime.h>
#include <timer.h>

// MCU wake up time in ticks
#define MIN_ALARM_DELAY     (3)
#define ALARM_RESOLUTION_MS ((uint32_t)10U)

#define TICK_DURATION_MS ((uint32_t)10U)
#define TICK_INTERVAL    AM_HAL_RTC_ALM_RPT_10TH

/*
 * Macros used by epoch time computation
 */
#define SUBSECONDS_IN_1SECOND ((uint32_t)100U)
#define TICKS_IN_1SUBSECOND   ((uint32_t)(ALARM_RESOLUTION_MS / TICK_DURATION_MS))
#define TICKS_IN_1SECOND      ((uint32_t)(1000U / TICK_DURATION_MS))
#define TICKS_IN_1DAY         ((uint32_t)(86400U * TICKS_IN_1SECOND))

#define DAYS_IN_MONTH_CORRECTION_NORM ((uint32_t)0x99AAA0)
#define DAYS_IN_MONTH_CORRECTION_LEAP ((uint32_t)0x445550)

static const uint8_t DaysInMonth[] = {31, 28, 31, 30, 31, 30,
                                      31, 31, 30, 31, 30, 31};

static const uint8_t DaysInMonthLeapYear[] = {31, 29, 31, 30, 31, 30,
                                              31, 31, 30, 31, 30, 31};

static const char *pcMonth[] = {
    "January",  "February", "March",        "April",     "May",
    "June",     "July",     "August",       "September", "October",
    "November", "December", "Invalid month"};

static bool    RtcInitialized           = false;
static bool    McuWakeUpTimeInitialized = false;
static int16_t McuWakeUpTimeCal         = 0;

typedef struct {
    bool     Running;
    uint64_t Ref_Ticks;
    uint64_t Alarm_Ticks;
} RtcTimerContext_t;

static RtcTimerContext_t RtcTimerContext;
static uint64_t          DeviceEpoch = 0LL;

static uint32_t RtcBackupRegisters[] = {0, 0};

static int toVal(char *pcAsciiStr)
{
    int iRetVal = 0;
    iRetVal += pcAsciiStr[1] - '0';
    iRetVal += pcAsciiStr[0] == ' ' ? 0 : (pcAsciiStr[0] - '0') * 10;
    return iRetVal;
}

static int mthToIndex(char *pcMon)
{
    int idx;
    for (idx = 0; idx < 12; idx++) {
        if (am_util_string_strnicmp(pcMonth[idx], pcMon, 3) == 0) {
            return idx;
        }
    }
    return 12;
}

static uint64_t RtcGetTicksSinceEpoch(am_hal_rtc_time_t *hal_time)
{
    uint64_t          ticks = 0LL;
    uint32_t          correction;
    uint64_t          seconds;
    am_hal_rtc_time_t local_hal_time;

    if (hal_time == NULL) {
        hal_time = &local_hal_time;
        am_hal_rtc_time_get(hal_time);
    }

    uint32_t year = hal_time->ui32Century * 100 + hal_time->ui32Year;
    /*
     * Replace the division by 4 with a bitshift
     *   DIVC(X, N) (((X) + (N)-1) / (N))
     *   seconds = DIVC((TM_DAYS_IN_YEAR * 3 + TM_DAYS_IN_LEAP_YEAR) * year, 4);
     */
    seconds = ((TM_DAYS_IN_YEAR * 3 + TM_DAYS_IN_LEAP_YEAR) * year + 3) >> 2;
    /*
     * Since year is positive, we can replace the modulo with a bitwise
     * operation for speed
     *   year % 4
     * is equivalent to (iff year is positive)
     *   year & 3
     * correction = ((year % 4) == 0) ? DAYS_IN_MONTH_CORRECTION_LEAP
     *                                : DAYS_IN_MONTH_CORRECTION_NORM;
     */
    correction = ((year & 0x03) == 0) ? DAYS_IN_MONTH_CORRECTION_LEAP
                                      : DAYS_IN_MONTH_CORRECTION_NORM;
    /*
     * Replace the division by 2 with a right bitshift
     * Replace the multiplication by 2 with a left bitshift
     *   DIVC(X, N) (((X) + (N)-1) / (N))
     *   seconds += (DIVC((hal_time->ui32Month) * (30 + 31), 2) -
     *              ((correction >> ((hal_time->ui32Month) * 2)) & 0x03));
     */
    seconds += ((((hal_time->ui32Month) * (30 + 31) - 1) >> 1) -
                ((correction >> ((hal_time->ui32Month) << 1)) & 0x03));
    seconds += hal_time->ui32DayOfMonth - 1;
    seconds *= TM_SECONDS_IN_1DAY;

    seconds += (((uint32_t)hal_time->ui32Hour * TM_SECONDS_IN_1HOUR) +
                ((uint32_t)hal_time->ui32Minute * TM_SECONDS_IN_1MINUTE) +
                (uint32_t)hal_time->ui32Second);

    ticks = (uint64_t)(seconds * TICKS_IN_1SECOND) +
            hal_time->ui32Hundredths * TICKS_IN_1SUBSECOND - DeviceEpoch;

    return ticks;
}

static void RtcAddTicks2Calendar(uint64_t ticks, am_hal_rtc_time_t *hal_time)
{
    uint32_t SubSeconds = 0;
    uint32_t Seconds    = 0;
    uint32_t Minutes    = 0;
    uint32_t Hours      = 0;
    uint32_t Days       = 0;

    uint32_t timeout         = ticks / TICKS_IN_1SECOND;
    uint32_t ticks_remainder = ticks - timeout * TICKS_IN_1SECOND;

    Days = hal_time->ui32DayOfMonth;
    while (timeout >= TM_SECONDS_IN_1DAY) {
        timeout -= TM_SECONDS_IN_1DAY;
        Days++;
    }

    Hours = hal_time->ui32Hour;
    while (timeout >= TM_SECONDS_IN_1HOUR) {
        timeout -= TM_SECONDS_IN_1HOUR;
        Hours++;
    }

    Minutes = hal_time->ui32Minute;
    while (timeout >= TM_SECONDS_IN_1MINUTE) {
        timeout -= TM_SECONDS_IN_1MINUTE;
        Minutes++;
    }

    Seconds = hal_time->ui32Second + timeout;

    if (TICKS_IN_1SUBSECOND == 0) {
        SubSeconds = hal_time->ui32Hundredths;
    } else {
        SubSeconds =
            hal_time->ui32Hundredths + ticks_remainder / TICKS_IN_1SUBSECOND;
    }

    while (SubSeconds >= SUBSECONDS_IN_1SECOND) {
        SubSeconds -= SUBSECONDS_IN_1SECOND;
        Seconds++;
    }

    while (Seconds >= TM_SECONDS_IN_1MINUTE) {
        Seconds -= TM_SECONDS_IN_1MINUTE;
        Minutes++;
    }

    while (Minutes >= TM_MINUTES_IN_1HOUR) {
        Minutes -= TM_MINUTES_IN_1HOUR;
        Hours++;
    }

    while (Hours >= TM_HOURS_IN_1DAY) {
        Hours -= TM_HOURS_IN_1DAY;
        Days++;
    }

    /*
     * while (Days >= DAYS_IN_1MONTH)
     * {
     *     Days -= DAYS_IN_1MONTH;
     *     Months++;
     * }
     */
    const uint8_t *DAYS_IN_1MONTH;
    uint32_t       year;

    year = hal_time->ui32Century * 100 + hal_time->ui32Year;
    if ((year & 0x03) == 0) {
        DAYS_IN_1MONTH = DaysInMonthLeapYear;
    } else {
        DAYS_IN_1MONTH = DaysInMonth;
    }

    while (Days >= DAYS_IN_1MONTH[hal_time->ui32Month]) {
        Days -= DAYS_IN_1MONTH[hal_time->ui32Month];
        hal_time->ui32Month++;
        if (hal_time->ui32Month > 11) {
            hal_time->ui32Month = 0;
            hal_time->ui32Year++;
            year = hal_time->ui32Century * 100 + hal_time->ui32Year;
            if ((year & 0x03) == 0) {
                DAYS_IN_1MONTH = DaysInMonthLeapYear;
            } else {
                DAYS_IN_1MONTH = DaysInMonth;
            }
        }
    }

    hal_time->ui32DayOfMonth = Days;
    hal_time->ui32Hour       = Hours;
    hal_time->ui32Minute     = Minutes;
    hal_time->ui32Second     = Seconds;
    hal_time->ui32Hundredths = SubSeconds;
}

void RtcInit(void)
{
    am_hal_rtc_time_t RtcHalTime;

    if (RtcInitialized == false) {
        am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_XTAL_START, 0);
        am_hal_rtc_osc_select(AM_HAL_RTC_OSC_XT);
        am_hal_rtc_osc_enable();

        RtcHalTime.ui32Hour       = toVal(&__TIME__[0]); // 0 to 23
        RtcHalTime.ui32Minute     = toVal(&__TIME__[3]); // 0 to 59
        RtcHalTime.ui32Second     = toVal(&__TIME__[6]); // 0 to 59
        RtcHalTime.ui32Hundredths = 00;
        RtcHalTime.ui32Weekday    = am_util_time_computeDayofWeek(
            2000 + toVal(&__DATE__[9]), mthToIndex(&__DATE__[0]) + 1,
            toVal(&__DATE__[4]));                          // 1 to 7
        RtcHalTime.ui32DayOfMonth = toVal(&__DATE__[4]);      // 1 to 31
        RtcHalTime.ui32Month      = mthToIndex(&__DATE__[0]); // 0 to 11
        RtcHalTime.ui32Year       = toVal(&__DATE__[9]); // years since 2000
        RtcHalTime.ui32Century    = 0;

        RtcTimerContext.Running = false;

        am_hal_rtc_time_set(&RtcHalTime);
        am_hal_rtc_alarm_interval_set(TICK_INTERVAL);
        am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
        am_hal_rtc_int_disable(AM_HAL_RTC_INT_ALM);
        NVIC_EnableIRQ(RTC_IRQn);

        DeviceEpoch = RtcGetTicksSinceEpoch(NULL);
        RtcSetTimerContext();

        RtcInitialized = true;
    }
}

uint32_t RtcGetMinimumTimeout(void) { return MIN_ALARM_DELAY; }

uint32_t RtcMs2Tick(uint32_t milliseconds)
{
    return (uint32_t)(milliseconds / TICK_DURATION_MS);
}

uint32_t RtcTick2Ms(uint32_t tick)
{
    return (uint32_t)(tick * TICK_DURATION_MS);
}

void RtcDelayMs(uint32_t delay)
{
    uint32_t refTicks   = RtcGetTimerValue();
    uint32_t delayTicks = RtcMs2Tick(delay);

    while ((RtcGetTimerValue() - refTicks) < delayTicks) {
        __NOP();
    }
}

uint32_t RtcSetTimerContext(void)
{
    RtcTimerContext.Ref_Ticks = RtcGetTicksSinceEpoch(NULL);

    return (uint32_t)RtcTimerContext.Ref_Ticks;
}

uint32_t RtcGetTimerContext(void) { return RtcTimerContext.Ref_Ticks; }

void RtcSetAlarm(uint32_t timeout) { RtcStartAlarm(timeout); }

void RtcStopAlarm(void)
{
    am_hal_rtc_int_disable(AM_HAL_RTC_INT_ALM);
    am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
    RtcTimerContext.Running = false;
}

void RtcStartAlarm(uint32_t timeout)
{
    am_hal_rtc_time_t alarm;

    RtcStopAlarm();

    am_hal_rtc_time_get(&alarm);
    RtcAddTicks2Calendar(timeout, &alarm);
    RtcTimerContext.Alarm_Ticks = RtcGetTicksSinceEpoch(&alarm);

    RtcTimerContext.Running = true;
    am_hal_rtc_alarm_set(&alarm, TICK_INTERVAL);
    am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
    am_hal_rtc_int_enable(AM_HAL_RTC_INT_ALM);
}

uint32_t RtcGetTimerValue(void)
{
    return (uint32_t)RtcGetTicksSinceEpoch(NULL);
}

uint32_t RtcGetTimerElapsedTime(void)
{
    uint64_t current = RtcGetTicksSinceEpoch(NULL);
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
    uint64_t ticks   = RtcGetTicksSinceEpoch(NULL);
    uint32_t seconds = (ticks + DeviceEpoch) / TICKS_IN_1SECOND;

    uint32_t ticks_remainder = ticks - seconds * TICKS_IN_1SECOND;
    *milliseconds            = RtcTick2Ms(ticks_remainder);

    return ticks * TICKS_IN_1SECOND;
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

void RtcProcess(void) {}

TimerTime_t RtcTempCompensation(TimerTime_t period, float temperature)
{
    return (TimerTime_t)period;
}

void am_rtc_isr(void)
{
    uint64_t current_ticks, alarm_ticks;

    am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);

    if (RtcTimerContext.Running) {
        current_ticks = RtcGetTicksSinceEpoch(NULL);
        alarm_ticks   = RtcTimerContext.Alarm_Ticks;

        if (current_ticks >= alarm_ticks) {
            RtcTimerContext.Running = false;
            TimerIrqHandler();
        }
    }
}
