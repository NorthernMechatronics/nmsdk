/******************************************************************************
 *
 * Filename     : console_task.h
 * Description  : Console task declarations
 *
 ******************************************************************************/
#if !defined(__CONSOLE_TASK_H__)
#define __CONSOLE_TASK_H__

#if defined(__cplusplus)
extern "C" {
#endif  // defined(__cplusplus)

#define CONSOLE_UART_INST 0

extern TaskHandle_t console_task_handle;

extern void g_console_task(void *pvp);
extern void g_console_print(const char *str);
extern void g_console_write(const char *str, size_t len);
extern char g_console_read_char();

#if defined(__cplusplus)
}
#endif  // defined(__cplusplus)

#endif  // !defined(__CONSOLE_TASK_H__)
