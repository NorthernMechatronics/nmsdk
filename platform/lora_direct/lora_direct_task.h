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
#ifndef __LORA_DIRECT_TASK_H__
#define __LORA_DIRECT_TASK_H__

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

typedef enum {
    IDLE,
    TX,
    RX,
    TXDONE,
    RXDONE,
    TIMEOUT,
    UNKNOWN
} lora_task_state_e;

extern TaskHandle_t lora_direct_task_handle;

extern void lora_direct_task(void *pvParameters);
extern void lora_direct_transmit_carrier(uint32_t frequency, uint8_t power);
extern void lora_direct_send(uint32_t frequency, uint8_t power,
                             const uint8_t *message, uint8_t length);
extern void lora_direct_receive(uint32_t frequency);
extern uint8_t lora_direct_message_subscribe(QueueHandle_t sTaskQueue,
                                             lora_task_state_e eEvent);
extern uint8_t lora_direct_message_unsubscribe(QueueHandle_t sTaskQueue,
                                               lora_task_state_e eEvent);

#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

#endif /* __LORA_DIRECT_TASK_H__ */
