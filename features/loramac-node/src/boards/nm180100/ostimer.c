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
#include <am_mcu_apollo.h>
#include <FreeRTOS.h>
#include "timers.h"
#include "timer.h"

#define TIMER_CALL_BLOCKTIME    5

static void vTimerCallback(TimerHandle_t xTimer)
{
    TimerEvent_t *obj = (TimerEvent_t *)pvTimerGetTimerID(xTimer);
    obj->IsStarted = false;
    obj->Callback(obj->Context);
}

void TimerInit( TimerEvent_t *obj, void ( *callback )( void *context ) )
{
    obj->Timestamp = 0;
    obj->ReloadValue = 0;
    obj->IsStarted = false;
    obj->IsNext2Expire = false;
    obj->Callback = callback;
    obj->Context = NULL;

    obj->Next = (TimerEvent_t *)xTimerCreate(
            "LoRaWAN",
            1,
            pdFALSE,
            (void *)obj,
            vTimerCallback);
}

void TimerSetContext( TimerEvent_t *obj, void* context )
{
    obj->Context = context;
}

void TimerStart( TimerEvent_t *obj )
{
    obj->IsStarted = true;
    xTimerStart((TimerHandle_t)obj->Next, TIMER_CALL_BLOCKTIME);
}

bool TimerIsStarted( TimerEvent_t *obj )
{
    return obj->IsStarted;
}

void TimerStop( TimerEvent_t *obj )
{
    xTimerStop((TimerHandle_t)obj->Next, TIMER_CALL_BLOCKTIME);
    obj->IsStarted = false;
}

void TimerReset( TimerEvent_t *obj )
{
    TimerStop( obj );
    TimerStart( obj );
}

void TimerSetValue( TimerEvent_t *obj, uint32_t value )
{
    obj->Timestamp = xTaskGetTickCount();
    obj->ReloadValue = value;
}

TimerTime_t TimerGetCurrentTime( void )
{
    return  xTaskGetTickCount();
}

TimerTime_t TimerGetElapsedTime( TimerTime_t past )
{
    if (past == 0)
    {
        return 0;
    }

    TickType_t now = xTaskGetTickCount();

    return (now - past);
}

TimerTime_t TimerTempCompensation( TimerTime_t period, float temperature )
{
    return period;
}

void TimerIrqHandler(void)
{

}

void TimerProcess( void )
{
}


