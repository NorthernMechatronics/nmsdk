/******************************************************************************
 *
 * Filename     : rtc-board.c
 * Description  : Ambiq RTC adaptation for LoRaMac-node
 *
 * Copyright (c) 2020 Joe Deu-Ngoc.  All rights reserved.
 *
 ******************************************************************************/

/******************************************************************************
 * Standard header files
 ******************************************************************************/
#include <string.h>
#include <stdbool.h>

/******************************************************************************
 * Ambiq Micro header files
 ******************************************************************************/
#include <am_mcu_apollo.h>
#include <am_bsp.h>
#include <am_util.h>

/******************************************************************************
 * Application Inludes
 ******************************************************************************/
#include <rtc-board.h>
#include <timer.h>


/******************************************************************************
 * Macros
 ******************************************************************************/
#define MIN_ALARM_DELAY                 (3)

/******************************************************************************
 * Type definitions
 ******************************************************************************/
typedef struct
{
  bool running;
  uint32_t ref_ticks;
  uint32_t alm_ticks;
}
rtc_timer_ctx_s;

typedef struct
{
  uint32_t data0;
  uint32_t data1;
}
rtc_backup_s;


/******************************************************************************
 * External functions
 ******************************************************************************/
extern void g_loramac_task_set_rtc_alarm_callback(void (*callback)(void));


/******************************************************************************
 * Local declarations
 ******************************************************************************/
static const uint8_t days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static bool rtc_initialized = false;
static rtc_timer_ctx_s rtc_timer_ctx;
static rtc_backup_s rtc_backup;


/******************************************************************************
 * Local functions
 ******************************************************************************
 * s_is_leap_year
 * --------------
 * Checks if given year is a leap-year.
 *
 * Parameters :
 *    @year   - Year to check
 *
 * Returns  :
 *    true if a leap-year, false otherwise
 ******************************************************************************/
static bool s_is_leap_year(uint32_t year)
{
  bool leap_year = false;

  if(year % 4 == 0)
  {
    if(year % 400 == 0)
    {
      leap_year = true;
    }
    else if(year % 100 == 0)
    {
      leap_year = false;
    }
    else
    {
      leap_year = true;
    }
  }

  return leap_year;
}

/******************************************************************************
 * s_get_rtc_ticks
 * ---------------
 * Get the number of RTC ticks from 2000/01/01 until given time
 *
 * Parameters :
 *    @t      - Ambiq RTC time
 *
 * Returns  :
 *    Ticks
 ******************************************************************************/
static uint32_t s_get_rtc_ticks(am_hal_rtc_time_t *t)
{
  uint32_t days, hours, minutes, seconds, ticks;
  uint32_t y, m, year;

  year = t->ui32Century * 100 + t->ui32Year;
  for(y = 0, days = 0; y < year; y++)
  {
    days += s_is_leap_year(y) ? 366 : 365;
  }
  for(m = 0; m < t->ui32Month; m++)
  {
    days += days_in_month[m];
  }
  if(m > 1 && s_is_leap_year(year))
  {
    days++;
  }
  days += (t->ui32DayOfMonth-1);
  hours = days*24 + t->ui32Hour;
  minutes = hours*60 + t->ui32Minute;
  seconds = minutes*60 + t->ui32Second;
  ticks = seconds*100 + t->ui32Hundredths;

  return ticks;
}

/******************************************************************************
 * s_handle_rtc_alarm
 * ------------------
 * Handle the alarm interrupts
 *
 ******************************************************************************/
static void s_handle_rtc_alarm(void)
{
  am_hal_rtc_time_t now;
  uint32_t ticks;

  am_hal_rtc_time_get(&now);
  ticks = s_get_rtc_ticks(&now);

  if(rtc_timer_ctx.running && ticks >= rtc_timer_ctx.alm_ticks)
  {
    rtc_timer_ctx.running = false;
    TimerIrqHandler();
  }
}

/******************************************************************************
 * Global functions
 ******************************************************************************
 * RtcInit
 * -------
 * Initializes the RTC code
 *
 ******************************************************************************/
void RtcInit(void)
{
  if(rtc_initialized == false)
  {
    rtc_timer_ctx.running = false;
    RtcSetTimerContext();
    g_loramac_task_set_rtc_alarm_callback(s_handle_rtc_alarm);
    rtc_initialized = true;
  }
}

/******************************************************************************
 * RtcGetMinimumTimeout
 * --------------------
 * Returns minimum timeout value
 *
 * Returns  :
 *    Minimum timeout value
 ******************************************************************************/
uint32_t RtcGetMinimumTimeout(void)
{
  return MIN_ALARM_DELAY;
}

/******************************************************************************
 * RtcMs2Tick
 * ----------
 * Convert milliseconds to ticks
 *
 * Parameters :
 *    @milliseconds - Milliseconds to convert
 *
 * Returns  :
 *    Number of ticks
 ******************************************************************************/
uint32_t RtcMs2Tick(TimerTime_t milliseconds)
{
  return (milliseconds + 9) / 10;
}

/******************************************************************************
 * RtcTick2Ms
 * ----------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
TimerTime_t RtcTick2Ms(uint32_t tick)
{
  return tick * 10;
}

/******************************************************************************
 * RtcDelayMs
 * ----------
 * duration by polling RTC
 *
 * Parameters :
 *    @milliseconds - Amount of time to delay
 *
 ******************************************************************************/
void RtcDelayMs(TimerTime_t milliseconds)
{
  uint32_t start = RtcGetTimerValue();
  uint32_t delay_ticks = RtcMs2Tick(milliseconds);

  while(((RtcGetTimerValue() - start)) < delay_ticks)
  {
    __NOP();
  }
}

/******************************************************************************
 * RtcSetAlarm
 * -----------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
void RtcSetAlarm(uint32_t timeout)
{
  RtcStartAlarm(timeout);
}

/******************************************************************************
 * RtcStopAlarm
 * ------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
void RtcStopAlarm(void)
{
  uint32_t mask = am_hal_interrupt_master_disable();

  rtc_timer_ctx.running = false;
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
  am_hal_rtc_int_disable(AM_HAL_RTC_INT_ALM);
  am_hal_interrupt_master_set(mask);
}

/******************************************************************************
 * RtcStartAlarm
 * -------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
void RtcStartAlarm(uint32_t timeout)
{
  uint32_t mask = am_hal_interrupt_master_disable();
  uint32_t ticks;
  am_hal_rtc_time_t now, alm;

  am_hal_rtc_time_get(&now);
  rtc_timer_ctx.ref_ticks = s_get_rtc_ticks(&now);
  ticks = rtc_timer_ctx.ref_ticks + timeout;
  rtc_timer_ctx.alm_ticks = ticks;

  memcpy(&alm, &now, sizeof(am_hal_rtc_time_t));
  alm.ui32Hundredths += timeout;

  rtc_timer_ctx.running = true;
  am_hal_rtc_alarm_set(&alm, AM_HAL_RTC_ALM_RPT_100TH);
  am_hal_rtc_int_clear(AM_HAL_RTC_INT_ALM);
  am_hal_rtc_int_enable(AM_HAL_RTC_INT_ALM);
  am_hal_interrupt_master_set(mask);
}

/******************************************************************************
 * RtcSetTimerContext
 * ------------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
uint32_t RtcSetTimerContext(void)
{
  am_hal_rtc_time_t now;
  am_hal_rtc_time_get(&now);
  rtc_timer_ctx.ref_ticks = s_get_rtc_ticks(&now);
  return rtc_timer_ctx.ref_ticks;
}

/******************************************************************************
 * RtcGetTimerContext
 * ------------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
uint32_t RtcGetTimerContext(void)
{
  return rtc_timer_ctx.ref_ticks;
}

/******************************************************************************
 * RtcGetCalendarTime
 * ------------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
uint32_t RtcGetCalendarTime(uint16_t *milliseconds)
{
  am_hal_rtc_time_t now;
  uint32_t ticks_now;

  am_hal_rtc_time_get(&now);
  ticks_now = s_get_rtc_ticks(&now);
  *milliseconds = ticks_now * 10;

  return ticks_now;
}

/******************************************************************************
 * RtcGetTimerValue
 * ----------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
uint32_t RtcGetTimerValue(void)
{
  am_hal_rtc_time_t now;
  uint32_t ticks_now;

  am_hal_rtc_time_get(&now);
  ticks_now = s_get_rtc_ticks(&now);

  return ticks_now;
}

/******************************************************************************
 * RtcGetTimerElapsedTime
 * ----------------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
uint32_t RtcGetTimerElapsedTime(void)
{
  am_hal_rtc_time_t now;
  uint32_t ticks_now;

  am_hal_rtc_time_get(&now);
  ticks_now = s_get_rtc_ticks(&now);

  return ticks_now - rtc_timer_ctx.ref_ticks;
}

/******************************************************************************
 * RtcBkupWrite
 * ------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
void RtcBkupWrite(uint32_t data0, uint32_t data1)
{
  rtc_backup.data0 = data0;
  rtc_backup.data1 = data1;
}

/******************************************************************************
 * RtcBkupRead
 * -----------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
void RtcBkupRead(uint32_t *data0, uint32_t *data1)
{
  if(data0)
  {
    *data0 = rtc_backup.data0;
  }
  if(data1)
  {
    *data1 = rtc_backup.data1;
  }
}

/******************************************************************************
 * RtcProcess
 * ----------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
void RtcProcess(void)
{
  // Do this stuff in the ISR
}

/******************************************************************************
 * RtcTempCompensation
 * -------------------
 * {Insert description here}
 *
 * Parameters :
 *    @period       -
 *    @temperature  -
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
TimerTime_t RtcTempCompensation(TimerTime_t period, float temperature)
{
  return period;
}