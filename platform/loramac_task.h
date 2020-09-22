/******************************************************************************
 *
 * Filename     : loramac_task.h
 * Description  : LoRaMac-node task
 *
 ******************************************************************************/
#if !defined(__LORAMAC_TASK_H__)
#define __LORAMAC_TASK_H__

#if defined(__cplusplus)
extern "C"
{
#endif // defined(__cplusplus)


/******************************************************************************
 * Global declarations
 ******************************************************************************/
extern TaskHandle_t loramac_task_handle;
extern const CLI_Command_Definition_t loramac_cmd_def;

/******************************************************************************
 * Global functions
 ******************************************************************************
 * g_loramac_task_set_rtc_alarm_callback
 * -------------------------------------
 * Register the callback function to call when RTC alarm fires
 *
 * Parameters   :
 *    @callback - Callback function to register
 *
 ******************************************************************************/
extern void g_loramac_task_set_rtc_alarm_callback(void (*callback)(void));

/******************************************************************************
 * g_loramac_isr_execute
 * ---------------------
 * Start the state machine
 *
 ******************************************************************************/
extern void g_loramac_isr_execute(void);

/******************************************************************************
 * g_loramac_task_setup
 * ---------------
 * LoRaMac task setup function
 *
 ******************************************************************************/
extern void g_loramac_task_setup(void);

/******************************************************************************
 * g_loramac_task
 * --------------
 * LoRaMac task entrypoint function
 *
 * Parameters :
 *  @pvp      - FreeRTOS parameters
 *
 ******************************************************************************/
extern void g_loramac_task(void *pvp);


#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

#endif // defined(__LORAMAC_TASK_H__)
