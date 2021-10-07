/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, Northern Mechatronics, Inc.
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

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <am_mcu_apollo.h>
#include "secure_store.h"

#define SECURE_STORE_UNLOCK_CMD_ADDR  0x40030078
#define SECURE_STORE_UNLOCK_KEY_ADDR  0x40030080

extern void secure_store_master_key_read(uint8_t *key) __attribute((weak, alias("secure_store_default_master_key_read")));

void secure_store_default_master_key_read(uint8_t *key)
{
    memset(key, 0xFF, MASTER_KEY_SIZE);
}

uint8_t secure_store_master_key_status()
{
    uint8_t master_key[MASTER_KEY_SIZE];

    secure_store_master_key_read(master_key);

    for (int i = 0; i < MASTER_KEY_SIZE; i++)
    {
        if (master_key[i] != 0xFF)
            return 1;
    }

    // erase the RAM to prevent in memory sniffing
    memset(master_key, 0, MASTER_KEY_SIZE);

    return 0;
}

void secure_store_unlock()
{
    uint8_t master_key[MASTER_KEY_SIZE];
    uint32_t *address, *value;

    secure_store_master_key_read(master_key);
    address = (uint32_t *)SECURE_STORE_UNLOCK_CMD_ADDR;
    am_hal_flash_store_ui32(address, 0x01);

    address = (uint32_t *)SECURE_STORE_UNLOCK_KEY_ADDR;
    for (int i = 0; i < MASTER_KEY_SIZE; i += 4)
    {
        value = (uint32_t *)(&(master_key[i]));
        am_hal_flash_store_ui32(address, *value);
        address++;
    }
 
    // erase the RAM to prevent in memory sniffing
    memset(master_key, 0, MASTER_KEY_SIZE);
}

void secure_store_unlock_with_key(uint8_t *master_key)
{
    uint32_t *address, *value;

    secure_store_master_key_read(master_key);
    address = (uint32_t *)SECURE_STORE_UNLOCK_CMD_ADDR;
    am_hal_flash_store_ui32(address, 0x01);

    address = (uint32_t *)SECURE_STORE_UNLOCK_KEY_ADDR;
    for (int i = 0; i < MASTER_KEY_SIZE; i += 4)
    {
        value = (uint32_t *)(&(master_key[i]));
        am_hal_flash_store_ui32(address, *value);
        address++;
    }
}

void secure_store_lock()
{
    uint32_t *address;

    address = (uint32_t *)SECURE_STORE_UNLOCK_KEY_ADDR;
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
        uint32_t value = am_hal_flash_load_ui32((uint32_t *)src);
        memcpy(dest, (uint8_t *)(&value), 4);

        src  += 4;
        dest += 4;
    }

    secure_store_lock();
}
