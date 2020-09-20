/*
 * iom.h
 *
 *  Created on: Jul 20, 2019
 *      Author: joshua
 */

#ifndef _IOM_IOM_H_
#define _IOM_IOM_H_

#if defined(__cplusplus)
extern "C" {
#endif  // defined(__cplusplus)

extern TaskHandle_t iom_task_handle;

void g_iom_task(void *pvp);

#if defined(__cplusplus)
}
#endif  // defined(__cplusplus)

#endif /* _IOM_IOM_H_ */
