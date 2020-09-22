/*
 * iom.h
 *
 *  Created on: Jul 20, 2019
 *      Author: joshua
 */

#ifndef _GPIO_H_
#define _GPIO_H_

#if defined(__cplusplus)
extern "C" {
#endif  // defined(__cplusplus)

extern TaskHandle_t gpio_task_handle;

void g_gpio_task(void *pvp);

#if defined(__cplusplus)
}
#endif  // defined(__cplusplus)

#endif /* _GPIO_H_ */
