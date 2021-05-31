/*************************************************************************************************/
/*!
 *  \file   wsf_timer.c
 *
 *  \brief  Timer service.
 *
 *  Copyright (c) 2009-2019 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019-2020 Packetcraft, Inc.
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#include <string.h>

#include "FreeRTOS.h"
#include "timers.h"

#include "wsf_types.h"
#include "wsf_queue.h"
#include "wsf_timer.h"

#include "wsf_assert.h"
#include "wsf_cs.h"
#include "wsf_trace.h"

/**************************************************************************************************
  Macros
**************************************************************************************************/
/* convert seconds to timer ticks */
#define WSF_TIMER_SEC_TO_TICKS(sec)         ((1000 / WSF_MS_PER_TICK) * (sec))

/* convert milliseconds to timer ticks */
#define WSF_TIMER_MS_TO_TICKS(ms)           ((ms) / WSF_MS_PER_TICK)

#define CLK_TICKS_PER_WSF_TICKS             (WSF_MS_PER_TICK*configTICK_RATE_HZ/1000)

/**************************************************************************************************
  Global Variables
**************************************************************************************************/

wsfQueue_t  wsfTimerTimerQueue;     /*!< Timer queue */
TimerHandle_t xWsfTimer;
static uint32_t wsfTimerLastTicks = 0;

static void WsfTimer_handler(TimerHandle_t xTimer)
{
  WsfTaskSetReady(0, WSF_TIMER_EVENT);
}

static void wsfTimerInsert(wsfTimer_t *pTimer, wsfTimerTicks_t ticks)
{
  wsfTimer_t  *pElem;
  wsfTimer_t  *pPrev = NULL;

  /* task schedule lock */
  WsfTaskLock();

  /* if timer is already running stop it first */
  if (pTimer->isStarted)
  {
    wsfTimerRemove(pTimer);
  }

  pTimer->isStarted = TRUE;
  pTimer->ticks = ticks;

  pElem = (wsfTimer_t *) wsfTimerTimerQueue.pHead;

  /* find insertion point in queue */
  while (pElem != NULL)
  {
    if (pTimer->ticks < pElem->ticks)
    {
      break;
    }
    pPrev = pElem;
    pElem = pElem->pNext;
  }

  /* insert timer into queue */
  WsfQueueInsert(&wsfTimerTimerQueue, pTimer, pPrev);

  /* task schedule unlock */
  WsfTaskUnlock();
}

/*************************************************************************************************/
/*!
 *  \brief  Initialize the timer service.  This function should only be called once
 *          upon system initialization.
 */
/*************************************************************************************************/
void WsfTimerInit(void)
{
  WSF_QUEUE_INIT(&wsfTimerTimerQueue);

  if(xWsfTimer == NULL)
  {
    xWsfTimer = xTimerCreate("WSF Timer", pdMS_TO_TICKS(WSF_MS_PER_TICK),
          pdFALSE, NULL, WsfTimer_handler);
    configASSERT(xWsfTimer);
    wsfTimerLastTicks = xTaskGetTickCount();
  }
}

/*************************************************************************************************/
/*!
 *  \brief  Start a timer in units of seconds.
 *
 *  \param  pTimer  Pointer to timer.
 *  \param  sec     Seconds until expiration.
 */
/*************************************************************************************************/
void WsfTimerStartSec(wsfTimer_t *pTimer, wsfTimerTicks_t sec)
{
  WSF_TRACE_INFO2("WsfTimerStartSec pTimer:0x%x ticks:%u", (uint32_t)pTimer, WSF_TIMER_SEC_TO_TICKS(sec));

  /* insert timer into queue */
  wsfTimerInsert(pTimer, WSF_TIMER_SEC_TO_TICKS(sec));
}

/*************************************************************************************************/
/*!
 *  \brief  Start a timer in units of milliseconds.
 *
 *  \param  pTimer  Pointer to timer.
 *  \param  ms      Milliseconds until expiration.
 */
/*************************************************************************************************/
void WsfTimerStartMs(wsfTimer_t *pTimer, wsfTimerTicks_t ms)
{
  WSF_TRACE_INFO2("WsfTimerStartMs pTimer:0x%x ticks:%u", (uint32_t)pTimer, WSF_TIMER_MS_TO_TICKS(ms));

  /* insert timer into queue */
  wsfTimerInsert(pTimer, WSF_TIMER_MS_TO_TICKS(ms));
}

/*************************************************************************************************/
/*!
 *  \brief  Stop a timer.
 *
 *  \param  pTimer  Pointer to timer.
 */
/*************************************************************************************************/
void WsfTimerStop(wsfTimer_t *pTimer)
{
    WSF_TRACE_INFO1("WsfTimerStop pTimer:0x%x", pTimer);

  /* task schedule lock */
  WsfTaskLock();

  wsfTimerRemove(pTimer);

  /* task schedule unlock */
  WsfTaskUnlock();
}

/*************************************************************************************************/
/*!
 *  \brief  Update the timer service with the number of elapsed ticks.
 *
 *  \param  ticks  Number of ticks since last update.
 */
/*************************************************************************************************/
void WsfTimerUpdate(wsfTimerTicks_t ticks)
{
  wsfTimer_t  *pElem;

  /* task schedule lock */
  WsfTaskLock();

  pElem = (wsfTimer_t *) wsfTimerTimerQueue.pHead;

  /* iterate over timer queue */
  while (pElem != NULL)
  {
    /* decrement ticks while preventing underflow */
    if (pElem->ticks > ticks)
    {
      pElem->ticks -= ticks;
    }
    else
    {
      pElem->ticks = 0;

      /* timer expired; set task for this timer as ready */
      WsfTaskSetReady(pElem->handlerId, WSF_TIMER_EVENT);
    }

    pElem = pElem->pNext;
  }

  /* task schedule unlock */
  WsfTaskUnlock();
}

/*************************************************************************************************/
/*!
 *  \brief  Service expired timers for the given task.
 *
 *  \param  taskId      Task ID.
 *
 *  \return Pointer to timer or NULL.
 */
/*************************************************************************************************/
wsfTimer_t *WsfTimerServiceExpired(wsfTaskId_t taskId)
{
  wsfTimer_t  *pElem;
  wsfTimer_t  *pPrev = NULL;

  /* Unused parameters */
  (void)taskId;

  /* task schedule lock */
  WsfTaskLock();

  /* find expired timers in queue */
  if (((pElem = (wsfTimer_t *) wsfTimerTimerQueue.pHead) != NULL) &&
      (pElem->ticks == 0))
  {
    /* remove timer from queue */
    WsfQueueRemove(&wsfTimerTimerQueue, pElem, pPrev);

    pElem->isStarted = FALSE;

    /* task schedule unlock */
    WsfTaskUnlock();

    WSF_TRACE_INFO1("Timer expired pTimer:0x%x", pElem);

    /* return timer */
    return pElem;
  }

  /* task schedule unlock */
  WsfTaskUnlock();

  return NULL;
}

/*************************************************************************************************/
/*!
 *  \brief  Function for checking if there is an active timer and if there is enough time to
 *          go to sleep and going to sleep.
 */
/*************************************************************************************************/
void WsfTimerSleep(void)
{
    uint32_t ui32CurrentTime, ui32ElapsedTime;
    bool_t bTimerRunning;
    wsfTimerTicks_t xNextExpiration;

    //
    // Read the continuous timer.
    //
    ui32CurrentTime = xTaskGetTickCount();

    //
    // Figure out how long it has been since the last time we've read the
    // continuous timer. We should be reading often enough that we'll never
    // have more than one overflow.
    //
    ui32ElapsedTime = ui32CurrentTime - wsfTimerLastTicks;

    //
    // Check to see if any WSF ticks need to happen.
    //
    if ( (ui32ElapsedTime / CLK_TICKS_PER_WSF_TICKS) > 0 )
    {
        //
        // Update the WSF timers and save the current time as our "last
        // update".
        //
        WsfTimerUpdate(ui32ElapsedTime / CLK_TICKS_PER_WSF_TICKS);

        wsfTimerLastTicks = ui32CurrentTime;
    }

    //
    // Check to see when the next timer expiration should happen.
    //
    xNextExpiration = WsfTimerNextExpiration(&bTimerRunning);

    //
    // If there's a pending WSF timer event, set an interrupt to wake us up in
    // time to service it.
    //
    if ( xNextExpiration )
    {
        configASSERT(pdPASS == xTimerChangePeriod( xWsfTimer,
                pdMS_TO_TICKS(xNextExpiration*CLK_TICKS_PER_WSF_TICKS), 100)) ;
    }
}


/*************************************************************************************************/
/*!
 *  \brief  Function for updating WSF timer based on elapsed RTC ticks.
 */
/*************************************************************************************************/
void WsfTimerSleepUpdate(void)
{
}
