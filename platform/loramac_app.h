/******************************************************************************
 *
 * Filename     : loramac_app.h
 * Description  : LoRaMac-node application declarations
 *
 ******************************************************************************/
#if !defined(__LORAMAC_APP_H__)
#define __LORAMAC_APP_H__

/******************************************************************************
 * Macros
 ******************************************************************************/
#define MAX_APP_DATA_SIZE               (242)
#define DEFAULT_LORAMAC_ADR_MODE        (1)
#define DEFAULT_LORAMAC_APP_PORT        (2)
#define DEFAULT_LORAMAC_CONFIRMED_MODE  false
#define DEFAULT_LORAMAC_DATARATE        DR_0


/******************************************************************************
 * Type definitions
 ******************************************************************************/
typedef enum
{
  APP_STATE_NULL,
  APP_STATE_INIT,
  APP_STATE_JOIN,
  APP_STATE_SEND,
  APP_STATE_SCHEDULE,
  APP_STATE_IDLE
}
loramac_app_state_e;

typedef void (*loramac_app_buf_done_fn)(void *buf, bool normal);


#if defined(__cplusplus)
extern "C"
{
#endif // defined(__cplusplus)


/******************************************************************************
 * Global functions
 ******************************************************************************
 * g_loramac_init
 * ----------------------
 * Startup the LoRaMac-node in Class A operation
 *
 * Parameters :
 *    @region - LoRaWAN region
 *
 * Returns  :
 *    true, if successful initialization.  false, otherwise.
 *
 ******************************************************************************/
extern bool g_loramac_init(LoRaMacRegion_t region);

/******************************************************************************
 * g_loramac_reset
 * ---------------
 * Resets LoRaMac-node
 *
 ******************************************************************************/
extern void g_loramac_reset(void);

/******************************************************************************
 * g_loramac_state_machine
 * -------------------------------
 * Execute the LoRaMac-node Class A State Machine
 *
 * Returns  :
 *    Next application state
 *
 ******************************************************************************/
extern loramac_app_state_e g_loramac_state_machine(void);

/******************************************************************************
 * g_loramac_get_buffer
 * --------------------
 * Get the next available application buffer.
 *
 * Parameters :
    @len      - [OUT] Size of buffer

 Returns  :
    Pointer to buffer

 ******************************************************************************/
extern void *g_loramac_get_buffer(size_t *len);

/******************************************************************************
 * g_loramac_enqueue_uplink
 * ------------------------
 * Enqueue a packet for uplink.
 *
 * Parameters     :
 *    @ack_mode  - LoRaWAN data transfer requires confirmation
 *    @port       - LoRaWAN port
 *    @buf        - Buffer to send
 *    @len        - Length of data to send
 *    @free       - Callback for freeing buffers at end of transmission
 *
 ******************************************************************************/
extern void g_loramac_enqueue_uplink(bool ack_mode, uint8_t port, uint8_t *buf, uint8_t len, loramac_app_buf_done_fn done);


#if defined(__cplusplus)
}
#endif // defined(__cplusplus)

#endif // defined(__LORAMAC_APP_H__)
