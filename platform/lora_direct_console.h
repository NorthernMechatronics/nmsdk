/*
 * lora_console.h
 *
 *  Created on: Oct 24, 2019
 *      Author: joshua
 */

#ifndef _LORA_CONSOLE_H_
#define _LORA_CONSOLE_H_

#if defined(__cplusplus)
extern "C" {
#endif  // defined(__cplusplus)

extern TaskHandle_t lora_direct_console_task_handle;

void g_lora_direct_console_task(void *pvp);

#if defined(__cplusplus)
}
#endif  // defined(__cplusplus)

#endif /* _LORA_CONSOLE_H_ */
