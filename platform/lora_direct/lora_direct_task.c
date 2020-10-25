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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <FreeRTOS.h>
#include <queue.h>
#include <task.h>

#include <am_mcu_apollo.h>
#include <nm_devices_lora.h>

#include "task_message.h"

#include "lora_direct_config.h"
#include "lora_direct_task.h"

typedef struct {
    QueueHandle_t sTaskQueue;
    uint32_t ui32Event;
} lora_direct_subscriber_t;

TaskHandle_t lora_direct_task_handle;

#define LORA_TASK_MESSAGE_QUEUE_SIZE 10
static QueueHandle_t gsLoRaTaskQueue;

static lora_radio_physical_packet_t gsLoRaRadioPhysicalPacket;
static uint8_t
    psLoRaRadioPhysicalPacketPayload[LORA_RADIO_MAX_PHYSICAL_PACKET] = {0};

#define MAX_SUBSCRIBERS 10
static uint8_t gui8LoRaMessageSubscriberSize;
static lora_direct_subscriber_t psLoRaMessageSubscriberList[MAX_SUBSCRIBERS] = {
    0};

void lora_direct_transmit_carrier(uint32_t frequency, uint8_t power)
{
    lora_radio_transfer_t transaction;

    transaction.ui32Frequency = frequency;
    transaction.ui8Power = power;
    transaction.eMode = LORA_RADIO_TXCARRIER;

    taskENTER_CRITICAL();
    lora_radio_transfer(NULL, &transaction);
    taskEXIT_CRITICAL();
}

void lora_direct_send(uint32_t frequency, uint8_t power, const uint8_t *message,
                      uint8_t length)
{
    lora_radio_transfer_t transaction;

    transaction.psModulationParameters = &gsLoRaModulationParameter;
    transaction.psPacketParameters = &gsLoRaPacketParameter;
    transaction.ui32Frequency = frequency;
    transaction.eMode = LORA_RADIO_TX;
    transaction.ui8Power = power;
    transaction.ui32Timeout = 0xFFFFFF00;
    transaction.ui32SyncWord = lora_radio_syncword;

    transaction.pui8Payload = (uint8_t *)message;
    gsLoRaPacketParameter.ui8PayloadLength = length;

    taskENTER_CRITICAL();
    lora_radio_transfer(NULL, &transaction);
    taskEXIT_CRITICAL();
}

void lora_direct_receive(uint32_t frequency)
{
    lora_radio_transfer_t transaction;

    transaction.psModulationParameters = &gsLoRaModulationParameter;
    transaction.psPacketParameters = &gsLoRaPacketParameter;
    transaction.ui32Frequency = frequency;
    transaction.eMode = LORA_RADIO_RX;
    transaction.ui32Timeout = 0xFFFFFF04;
    transaction.ui32SyncWord = lora_radio_syncword;

    taskENTER_CRITICAL();
    lora_radio_transfer(NULL, &transaction);
    taskEXIT_CRITICAL();
}

uint8_t lora_direct_message_subscribe(QueueHandle_t sTaskQueue,
                                      lora_task_state_e eEvent)
{
    if (gui8LoRaMessageSubscriberSize >= MAX_SUBSCRIBERS) {
        return 0;
    }

    psLoRaMessageSubscriberList[gui8LoRaMessageSubscriberSize].sTaskQueue =
        sTaskQueue;
    psLoRaMessageSubscriberList[gui8LoRaMessageSubscriberSize].ui32Event =
        eEvent;

    gui8LoRaMessageSubscriberSize++;

    return 1;
}

uint8_t lora_direct_message_unsubscribe(QueueHandle_t sTaskQueue,
                                        lora_task_state_e eEvent)
{
    uint8_t i = 0;
    uint8_t ui8Found = 0;

    for (i = 0; i < gui8LoRaMessageSubscriberSize; i++) {
        if ((psLoRaMessageSubscriberList[i].sTaskQueue == sTaskQueue) &&
            (psLoRaMessageSubscriberList[i].ui32Event == eEvent)) {
            ui8Found = 1;
            break;
        }
    }

    if (ui8Found) {
        while (i < (gui8LoRaMessageSubscriberSize - 1)) {
            psLoRaMessageSubscriberList[i].sTaskQueue =
                psLoRaMessageSubscriberList[i + 1].sTaskQueue;
            psLoRaMessageSubscriberList[i].ui32Event =
                psLoRaMessageSubscriberList[i + 1].ui32Event;
            i++;
        }
        gui8LoRaMessageSubscriberSize--;

        return 1;
    }

    return 0;
}

static void lora_direct_notify(uint32_t state, void *content)
{
    task_message_t sTaskMessage;

    sTaskMessage.ui32Event = state;
    sTaskMessage.psContent = content;

    for (uint8_t i = 0; i < gui8LoRaMessageSubscriberSize; i++) {
        if (psLoRaMessageSubscriberList[i].ui32Event == state) {
            xQueueSend(psLoRaMessageSubscriberList[i].sTaskQueue, &sTaskMessage,
                       portMAX_DELAY);
        }
    }
}

static void lora_direct_callback_txdone(void *arg)
{
    task_message_t sTaskMessage;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    sTaskMessage.ui32Event = TXDONE;
    sTaskMessage.psContent = NULL;

    xQueueSendFromISR(gsLoRaTaskQueue, &sTaskMessage,
                      &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void lora_direct_callback_rxdone(void *arg)
{
    lora_radio_physical_packet_t *content = (lora_radio_physical_packet_t *)arg;

    gsLoRaRadioPhysicalPacket.i8Rscp = content->i8Rscp;
    gsLoRaRadioPhysicalPacket.i8Rssi = content->i8Rssi;
    gsLoRaRadioPhysicalPacket.i8Snr = content->i8Snr;
    gsLoRaRadioPhysicalPacket.ui8PayloadLength = content->ui8PayloadLength;
    gsLoRaRadioPhysicalPacket.pui8Payload = psLoRaRadioPhysicalPacketPayload;
    memcpy(psLoRaRadioPhysicalPacketPayload, content->pui8Payload,
           content->ui8PayloadLength);

    task_message_t sTaskMessage;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    sTaskMessage.ui32Event = RXDONE;
    sTaskMessage.psContent = &gsLoRaRadioPhysicalPacket;

    xQueueSendFromISR(gsLoRaTaskQueue, &sTaskMessage,
                      &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void lora_direct_callback_timeout(void *arg)
{
    task_message_t sTaskMessage;
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;

    sTaskMessage.ui32Event = TIMEOUT;
    sTaskMessage.psContent = NULL;

    xQueueSendFromISR(gsLoRaTaskQueue, &sTaskMessage,
                      &xHigherPriorityTaskWoken);

    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
}

static void lora_direct_task_init(void)
{
    memset(psLoRaMessageSubscriberList, 0,
           MAX_SUBSCRIBERS * sizeof(lora_direct_subscriber_t));
    gui8LoRaMessageSubscriberSize = 0;

    taskENTER_CRITICAL();
    lora_radio_callback_list_init();

    lora_radio_callback_register(LORA_RADIO_TXDONE,
                                 &lora_direct_callback_txdone);
    lora_radio_callback_register(LORA_RADIO_RXDONE,
                                 &lora_direct_callback_rxdone);
    lora_radio_callback_register(LORA_RADIO_TIMEOUT,
                                 &lora_direct_callback_timeout);
    taskEXIT_CRITICAL();

    lora_direct_radio_configuration_reset();
    lora_radio_initialize(NULL);
    lora_direct_receive(lora_radio_frequency);
}

void lora_direct_task(void *pvParameters)
{
    task_message_t sTaskMessage;

    gsLoRaTaskQueue =
        xQueueCreate(LORA_TASK_MESSAGE_QUEUE_SIZE, sizeof(task_message_t));

    lora_direct_task_init();

    while (1) {
        if (xQueueReceive(gsLoRaTaskQueue, &sTaskMessage, portMAX_DELAY) ==
            pdPASS) {
            switch (sTaskMessage.ui32Event) {
            case TXDONE: {
                lora_direct_notify(sTaskMessage.ui32Event, NULL);
                lora_direct_receive(lora_radio_frequency);
            } break;
            case RXDONE: {
                // no need for deep copy as content is already statically allocated in psLoRaRadioPhysicalPacketPayload
                lora_radio_physical_packet_t *content =
                    (lora_radio_physical_packet_t *)sTaskMessage.psContent;
                lora_direct_notify(sTaskMessage.ui32Event, content);
                lora_direct_receive(lora_radio_frequency);
            } break;
            case TIMEOUT: {
                lora_direct_notify(sTaskMessage.ui32Event, NULL);
                lora_direct_receive(lora_radio_frequency);
            } break;
            }
        }
    }
}
