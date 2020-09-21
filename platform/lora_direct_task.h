/*
 * lora_direct_task.h
 *
 *  Created on: Jul 22, 2019
 *      Author: joshua
 */

#ifndef __LORA_DIRECT_TASK_H__
#define __LORA_DIRECT_TASK_H__

#if defined(__cplusplus)
extern "C" {
#endif  // defined(__cplusplus)

typedef enum
{
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
extern void lora_direct_send(uint32_t frequency, uint8_t power, const uint8_t *message, uint8_t length);
extern void lora_direct_receive(uint32_t frequency);
extern uint8_t lora_direct_message_subscribe(QueueHandle_t sTaskQueue, lora_task_state_e eEvent);
extern uint8_t lora_direct_message_unsubscribe(QueueHandle_t sTaskQueue, lora_task_state_e eEvent);

#if defined(__cplusplus)
}
#endif  // defined(__cplusplus)

#endif /* __LORA_DIRECT_TASK_H__ */
