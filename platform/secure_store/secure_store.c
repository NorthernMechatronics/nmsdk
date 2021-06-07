#include <am_mcu_apollo.h>
#include "secure_store_config.h"
#include "secure_store.h"

#define MASTER_KEY_SIZE 16
#define SECURE_STORE_UNLOCK_CMD_ADDR  0x40030078
#define SECURE_STORE_UNLOCK_KEY_ADDR  0x40030080

static void secure_store_master_key_read(uint8_t *key)
{
    uint32_t *address = key;
    uint32_t *source = MASTER_KEY_ADDRESS;
    for (int i = 0; i < MASTER_KEY_SIZE; i += 4)
    {
        uint32_t value = am_hal_flash_load_ui32(source);
        memcpy(address, (uint8_t *)(&value), 4);

        address++;
        source++;
    }
}

void secure_store_unlock()
{
    uint8_t master_key[MASTER_KEY_SIZE];
    uint32_t *address, *value;

    secure_store_master_key_read(master_key);
    address = SECURE_STORE_UNLOCK_CMD_ADDR;
    am_hal_flash_store_ui32(address, 0x01);

    address = SECURE_STORE_UNLOCK_KEY_ADDR;
    for (int i = 0; i < MASTER_KEY_SIZE; i += 4)
    {
        value = &(master_key[i]);
        am_hal_flash_store_ui32(address, *value);
        address++;
    }
}

void secure_store_lock()
{
    uint32_t *address;

    address = SECURE_STORE_UNLOCK_KEY_ADDR;
    for (int i = 0; i < MASTER_KEY_SIZE; i += 4)
    {
        am_hal_flash_store_ui32(address, 0xFFFFFFFF);
        address++;
    }
}

void secure_store_read(uint32_t addr, uint8_t *target, size_t len)
{
    uint8_t *src  = (uint8_t *)addr;
    uint8_t *dest = (uint8_t *)target;

    secure_store_unlock();

    for (int i = 0; i < len; i += 4)
    {
        uint32_t value = am_hal_flash_load_ui32(src);
        memcpy(dest, (uint8_t *)(&value), 4);

        src  += 4;
        dest += 4;
    }

    secure_store_lock();
}
