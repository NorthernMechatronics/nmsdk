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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include <am_hal_flash.h>

#include "eeprom_emulation.h"

#define MAX_NUMBER_OF_PAGES 4

#define SIZE_OF_DATA 2                                            /* 2 bytes */
#define SIZE_OF_VIRTUAL_ADDRESS 2                                 /* 2 bytes */
#define SIZE_OF_VARIABLE (SIZE_OF_DATA + SIZE_OF_VIRTUAL_ADDRESS) /* 4 bytes */

#define MAX_ACTIVE_VARIABLES (AM_HAL_FLASH_PAGE_SIZE / SIZE_OF_VARIABLE) - 1

typedef enum {
    EEPROM_PAGE_STATUS_ERASED = 0xFF,
    EEPROM_PAGE_STATUS_RECEIVING = 0xAA,
    EEPROM_PAGE_STATUS_ACTIVE = 0x00,
} eeprom_page_status_e;

typedef struct {
    uint32_t *pui32StartAddress;
    uint32_t *pui32EndAddress;
} eeprom_page_t;

/* Variables to keep track of what pages are active and receiving. */
static int activePageNumber = -1;
static int receivingPageNumber = -1;

static bool initialized = false;

/* Array of all pages allocated to the eeprom */
static eeprom_page_t pages[MAX_NUMBER_OF_PAGES];

static int16_t numberOfPagesAllocated;

/* Since the data to be written to flash must be read from ram, the data used to
 * set the pages' status, is explicitly written to the ram beforehand. */
static uint32_t EEPROM_PAGE_STATUS_ACTIVE_VALUE =
    ((uint32_t)EEPROM_PAGE_STATUS_ACTIVE << 24) | 0x00FFFFFF;
static uint32_t EEPROM_PAGE_STATUS_RECEIVING_VALUE =
    ((uint32_t)EEPROM_PAGE_STATUS_RECEIVING << 24) | 0x00FFFFFF;

static inline eeprom_page_status_e eeprom_page_get_status(eeprom_page_t *page)
{
    return (eeprom_page_status_e)((*(page->pui32StartAddress) >> 24) & 0xFF);
}

static inline int eeprom_page_set_active(eeprom_page_t *page)
{
    return am_hal_flash_program_main(
        AM_HAL_FLASH_PROGRAM_KEY, &EEPROM_PAGE_STATUS_ACTIVE_VALUE,
        page->pui32StartAddress, SIZE_OF_VARIABLE >> 2);
}

static inline int eeprom_page_set_receiving(eeprom_page_t *page)
{
    return am_hal_flash_program_main(
        AM_HAL_FLASH_PROGRAM_KEY, &EEPROM_PAGE_STATUS_RECEIVING_VALUE,
        page->pui32StartAddress, SIZE_OF_VARIABLE >> 2);
}

static bool eeprom_validate_empty(eeprom_page_t *page)
{
    uint32_t *address = page->pui32StartAddress;

    while (address <= page->pui32EndAddress) {
        if (*address != 0xFFFFFFFF) {
            return false;
        }
        address++;
    }

    return true;
}

static bool eeprom_page_write(eeprom_page_t *page, uint16_t virtual_address,
                              uint16_t data)
{
    /* Start at the second word. The fist one is reserved for status and erase count. */
    uint32_t *address = page->pui32StartAddress + 1;
    uint32_t virtualAddressAndData;

    /* Iterate through the page from the beginning, and stop at the fist empty word. */
    while (address <= page->pui32EndAddress)
    {
        /* Empty word found. */
        if (*address == 0xFFFFFFFF)
        {
            virtualAddressAndData =
                ((uint32_t)(virtual_address << 16) & 0xFFFF0000) |
                (uint32_t)(data);

            if (am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY,
                                          &virtualAddressAndData, address,
                                          SIZE_OF_VARIABLE >> 2) != 0)
            {
                return false;
            }
            return true;
        }
        else
        {
            address++;
        }
    }

    return false;
}

bool eeprom_format(uint32_t numberOfPages)
{
    uint32_t ui32EraseCount = 0xFF000001;
    int i;
    int status;

    numberOfPagesAllocated = numberOfPages;

    for (i = 0; i < numberOfPagesAllocated; i++)
    {
        pages[i].pui32StartAddress =
            (uint32_t *)(AM_HAL_FLASH_LARGEST_VALID_ADDR -
                         i * AM_HAL_FLASH_PAGE_SIZE - AM_HAL_FLASH_PAGE_SIZE +
                         1);
        pages[i].pui32EndAddress =
            (uint32_t *)(AM_HAL_FLASH_LARGEST_VALID_ADDR -
                         i * AM_HAL_FLASH_PAGE_SIZE - 4 + 1);
    }

    for (i = numberOfPagesAllocated - 1; i >= 0; i--)
    {
        if (!eeprom_validate_empty(&pages[i]))
        {
            status = am_hal_flash_page_erase(
                AM_HAL_FLASH_PROGRAM_KEY,
                AM_HAL_FLASH_ADDR2INST((uint32_t)(pages[i].pui32StartAddress)),
                AM_HAL_FLASH_ADDR2PAGE((uint32_t)(pages[i].pui32StartAddress)));
            if (status != 0)
            {
                return false;
            }
        }
    }

    activePageNumber = 0;

    receivingPageNumber = -1;

    status =
        am_hal_flash_program_main(AM_HAL_FLASH_PROGRAM_KEY, &ui32EraseCount,
                                  pages[activePageNumber].pui32StartAddress, 1);

    if (status != 0)
    {
        return false;
    }

    status = eeprom_page_set_active(&pages[activePageNumber]);
    if (status != 0)
    {
        return false;
    }

    return true;
}

static int eeprom_page_transfer(uint16_t virtual_address, uint16_t data)
{
    int status;
    uint32_t *pui32ActiveAddress;
    uint32_t *pui32ReceivingAddress;
    uint32_t ui32EraseCount;
    bool bNewData = false;

    /* If there is no receiving page predefined, set it to cycle through all allocated pages. */
    if (receivingPageNumber == -1)
    {
        receivingPageNumber = activePageNumber + 1;

        if (receivingPageNumber >= numberOfPagesAllocated)
        {
            receivingPageNumber = 0;
        }

        /* Check if the new receiving page really is erased. */
        if (!eeprom_validate_empty(&pages[receivingPageNumber]))
        {
            /* If this page is not truly erased, it means that it has been written to
             * from outside this API, this could be an address conflict. */
            am_hal_flash_page_erase(
                AM_HAL_FLASH_PROGRAM_KEY,
                AM_HAL_FLASH_ADDR2INST(
                    (uint32_t)(pages[receivingPageNumber].pui32StartAddress)),
                AM_HAL_FLASH_ADDR2PAGE(
                    (uint32_t)(pages[receivingPageNumber].pui32StartAddress)));
        }
    }

    /* Set the status of the receiving page */
    eeprom_page_set_receiving(&pages[receivingPageNumber]);

    /* If an address was specified, write it to the receiving page */
    if (virtual_address != 0)
    {
        eeprom_page_write(&pages[receivingPageNumber], virtual_address, data);
    }

    /* Start at the last word. */
    pui32ActiveAddress = pages[activePageNumber].pui32EndAddress;

    /* Iterate through all words in the active page. Each time a new virtual
     * address is found, write it and it's data to the receiving page */
    while (pui32ActiveAddress > pages[activePageNumber].pui32StartAddress)
    {
        // 0x0000 and 0xFFFF are not valid virtual addresses.
        if ((uint16_t)(*pui32ActiveAddress >> 16) == 0x0000 ||
            (uint16_t)(*pui32ActiveAddress >> 16) == 0xFFFF)
        {
            bNewData = false;
        }
        /*
    // Omit when transfer is initiated from inside the eeprom_init() function.
    else if (address != 0 && (uint16_t)(*pui32ActiveAddress >> 16) > numberOfVariablesDeclared)
    {
      // A virtual address outside the virtual address space, defined by the
      // number of variables declared, are considered garbage.
      newVariable = false;
    }
    */
        else
        {
            pui32ReceivingAddress =
                pages[receivingPageNumber].pui32StartAddress + 1;

            /* Start at the beginning of the receiving page. Check if the variable is
             * already transfered. */
            while (pui32ReceivingAddress <=
                   pages[receivingPageNumber].pui32EndAddress)
            {
                /* Variable found, and is therefore already transferred. */
                if ((uint16_t)(*pui32ActiveAddress >> 16) ==
                    (uint16_t)(*pui32ReceivingAddress >> 16))
                {
                    bNewData = false;
                    break;
                }
                /* Empty word found. All transferred variables are checked.  */
                else if (*pui32ReceivingAddress == 0xFFFFFFFF)
                {
                    bNewData = true;
                    break;
                }
                pui32ReceivingAddress++;
            }
        }

        if (bNewData)
        {
            /* Write the new variable to the receiving page. */
            eeprom_page_write(&pages[receivingPageNumber],
                              (uint16_t)(*pui32ActiveAddress >> 16),
                              (uint16_t)(*pui32ActiveAddress));
        }
        pui32ActiveAddress--;
    }

    /* Update erase count */
    ui32EraseCount = eeprom_erase_counter();

    /* If a new page cycle is started, increment the erase count. */
    if (receivingPageNumber == 0)
        ui32EraseCount++;

    /* Set the first byte, in this way the page status is not altered when the erase count is written. */
    ui32EraseCount = ui32EraseCount | 0xFF000000;

    /* Write the erase count obtained to the active page head. */
    status = am_hal_flash_program_main(
        AM_HAL_FLASH_PROGRAM_KEY, &ui32EraseCount,
        pages[receivingPageNumber].pui32StartAddress, SIZE_OF_VARIABLE >> 2);
    if (status != 0)
    {
        return status;
    }

    /* Erase the old active page. */
    status = am_hal_flash_page_erase(
        AM_HAL_FLASH_PROGRAM_KEY,
        AM_HAL_FLASH_ADDR2INST(
            (uint32_t)(pages[activePageNumber].pui32StartAddress)),
        AM_HAL_FLASH_ADDR2PAGE(
            (uint32_t)(pages[activePageNumber].pui32StartAddress)));
    if (status != 0)
    {
        return status;
    }

    /* Set the receiving page to be the new active page. */
    status = eeprom_page_set_active(&pages[receivingPageNumber]);
    if (status != 0)
    {
        return status;
    }

    activePageNumber = receivingPageNumber;
    receivingPageNumber = -1;

    return 0;
}

bool eeprom_init(uint32_t numberOfPages)
{
    if (initialized)
        return false;

    initialized = true;

    if (numberOfPages < 2)
    {
        numberOfPages = 2;
    }

    numberOfPagesAllocated = numberOfPages;

    /* Initialize the address of each page */
    uint32_t i;
    for (i = 0; i < numberOfPages; i++)
    {
        pages[i].pui32StartAddress =
            (uint32_t *)(AM_HAL_FLASH_LARGEST_VALID_ADDR -
                         i * AM_HAL_FLASH_PAGE_SIZE - AM_HAL_FLASH_PAGE_SIZE +
                         1);
        pages[i].pui32EndAddress =
            (uint32_t *)(AM_HAL_FLASH_LARGEST_VALID_ADDR -
                         i * AM_HAL_FLASH_PAGE_SIZE - 4 + 1);
    }

    /* Check status of each page */
    for (i = 0; i < numberOfPages; i++)
    {
        switch (eeprom_page_get_status(&pages[i]))
        {
        case EEPROM_PAGE_STATUS_ACTIVE:
            if (activePageNumber == -1) {
                activePageNumber = i;
            } else {
                // More than one active page found. This is an invalid system state.
                return false;
            }
            break;
        case EEPROM_PAGE_STATUS_RECEIVING:
            if (receivingPageNumber == -1) {
                receivingPageNumber = i;
            } else {
                // More than one receiving page found. This is an invalid system state.
                return false;
            }
            break;
        case EEPROM_PAGE_STATUS_ERASED:
            // Validate if the page is really erased, and erase it if not.
            if (!eeprom_validate_empty(&pages[i])) {
                am_hal_flash_page_erase(AM_HAL_FLASH_PROGRAM_KEY,
                                        AM_HAL_FLASH_ADDR2INST((uint32_t)(
                                            pages[i].pui32StartAddress)),
                                        AM_HAL_FLASH_ADDR2PAGE((uint32_t)(
                                            pages[i].pui32StartAddress)));
            }
            break;
        default:
            // Undefined page status, erase page.
            am_hal_flash_page_erase(
                AM_HAL_FLASH_PROGRAM_KEY,
                AM_HAL_FLASH_ADDR2INST((uint32_t)(pages[i].pui32StartAddress)),
                AM_HAL_FLASH_ADDR2PAGE((uint32_t)(pages[i].pui32StartAddress)));
            break;
        }
    }

    if (receivingPageNumber == -1 && activePageNumber == -1) {
        return false;
    }

    if (receivingPageNumber == -1) {
        return true;
    } else if (activePageNumber == -1) {
        activePageNumber = receivingPageNumber;
        receivingPageNumber = -1;
        eeprom_page_set_active(&pages[activePageNumber]);
    } else {
        eeprom_page_transfer(0, 0);
    }

    return true;
}

bool eeprom_read(uint16_t virtual_address, uint16_t *data)
{
    uint32_t *pui32Address;

    if (!initialized) {
        return false;
    }

    pui32Address = (pages[activePageNumber].pui32EndAddress);

    // 0x0000 and 0xFFFF are illegal addresses.
    if (virtual_address != 0x0000 && virtual_address != 0xFFFF) {
        while (pui32Address > pages[activePageNumber].pui32StartAddress) {
            if ((uint16_t)(*pui32Address >> 16) == virtual_address) {
                *data = (uint16_t)(*pui32Address);
                return true;
            }
            pui32Address--;
        }
    }
    // Variable not found, return null value.
    *data = 0x0000;

    return false;
}

bool eeprom_read_array(uint16_t virtual_address, uint8_t *data, uint8_t *len)
{
    uint16_t value;
    if (!initialized) {
        return false;
    }

    if (!eeprom_read(virtual_address, &value))
    {
        return false;
    }

    *len = (value >> 8) & 0xFF;
    data[0] = value & 0xFF;
    for (int i = 1; i < *len; i++)
    {
        if (!eeprom_read(virtual_address + i, &value))
        {
            *len = i;
            return false;
        }
        data[i] = value & 0xFF;
    }

    return true;
}

bool eeprom_read_array_len(uint16_t virtual_address, uint8_t *data, uint16_t size)
{
    uint16_t value;
    if (!initialized) {
        return false;
    }

    for (int i = 0; i < size; i++)
    {
        if (!eeprom_read(virtual_address + i, &value))
        {
            return false;
        }
        data[i] = value & 0xFF;
    }

    return true;
}

void eeprom_write(uint16_t virtual_address, uint16_t data)
{
    if (!initialized) {
        return;
    }

    uint16_t stored_value;

    if (eeprom_read(virtual_address, &stored_value)) {
        if (stored_value == data) {
            return;
        }
    }

    if (!eeprom_page_write(&pages[activePageNumber], virtual_address, data)) {
        eeprom_page_transfer(virtual_address, data);
    }
}

void eeprom_write_array(uint16_t virtual_address, uint8_t *data, uint8_t len)
{
    if (!initialized) {
        return;
    }

    uint16_t stored_value;

    if (eeprom_read(virtual_address, &stored_value)) {
        uint8_t stored_len = (stored_value >> 8) & 0xFF;
        if (stored_len != len) {
            for (int i = 0; i < stored_len; i++)
            {
                eeprom_delete(virtual_address + i);
            }
        }
    }

    uint16_t value = (len << 8) | data[0];
    if (!eeprom_page_write(&pages[activePageNumber], virtual_address, value)) {
        eeprom_page_transfer(virtual_address, value);
    }

    for (int i = 1; i < len; i++)
    {
        if (!eeprom_page_write(&pages[activePageNumber], virtual_address + i, data[i])) {
            eeprom_page_transfer(virtual_address + i, data[i]);
        }
    }
}

void eeprom_write_array_len(uint16_t virtual_address, uint8_t *data, uint16_t size)
{
    if (!initialized) {
        return;
    }

    for (int i = 0; i < size; i++)
    {
        eeprom_write(virtual_address + i, data[i]);
    }
}

bool eeprom_delete_array(uint16_t virtual_address)
{
    bool bDeleted = false;
    if (!initialized) {
        return false;
    }

    uint16_t stored_value;

    if (eeprom_read(virtual_address, &stored_value))
    {
        uint8_t stored_len = (stored_value >> 8) & 0xFF;
        for (int i = 0; i < stored_len; i++)
        {
            bDeleted |= eeprom_delete(virtual_address + i);
        }
    }

    return bDeleted;
}

bool eeprom_delete(uint16_t virtual_address)
{
    bool bDeleted = false;

    uint32_t data = 0x0000FFFF;
    uint32_t *address = pages[activePageNumber].pui32EndAddress;

    while (address > pages[activePageNumber].pui32StartAddress)
    {
        if ((uint16_t)(*address >> 16) == virtual_address)
        {
            bDeleted = true;
            am_hal_flash_program_main(
                  AM_HAL_FLASH_PROGRAM_KEY,
                  &data, address,
                  SIZE_OF_VARIABLE >> 2);
        }
        address--;
    }

    return bDeleted;
}

uint32_t eeprom_erase_counter(void)
{
    if (activePageNumber == -1)
    {
        return 0xFFFFFF;
    }

    uint32_t eraseCount;

    /* The number of erase cycles is the 24 LSB of the first word of the active page. */
    eraseCount = (*(pages[activePageNumber].pui32StartAddress) & 0x00FFFFFF);

    /* if the page has never been erased, return 0. */
    if (eraseCount == 0xFFFFFF) {
        return 0;
    }

    return eraseCount;
}
