#ifndef __TASK_MESSAGE_H__
#define __TASK_MESSAGE_H__

#include <stdint.h>
#include <FreeRTOS.h>
#include <queue.h>

typedef struct
{
	uint32_t  ui32Event;
	void     *psContent;
} task_message_t;

#endif /* __TASK_MESSAGE_H__ */
