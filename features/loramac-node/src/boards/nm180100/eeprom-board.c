/*!
 * \file      eeprom-board.c
 *
 * \brief     Target board EEPROM driver implementation
 *
 * \copyright Revised BSD License, see section \ref LICENSE.
 *
 * \code
 *                ______                              _
 *               / _____)             _              | |
 *              ( (____  _____ ____ _| |_ _____  ____| |__
 *               \____ \| ___ |    (_   _) ___ |/ ___)  _ \
 *               _____) ) ____| | | || |_| ____( (___| | | |
 *              (______/|_____)_|_|_| \__)_____)\____)_| |_|
 *              (C)2013-2017 Semtech
 *
 * \endcode
 *
 * \author    Miguel Luis ( Semtech )
 *
 * \author    Gregory Cristian ( Semtech )
 */
#include "utilities.h"
#include "eeprom-board.h"
#include "eeprom_emulation.h"
#include "eeprom_emulation_conf.h"

void EepromMcuInit(void)
{
    if (!eeprom_init(EEPROM_EMULATION_FLASH_PAGES))
    {
        eeprom_format(EEPROM_EMULATION_FLASH_PAGES);
    }
}

uint8_t EepromMcuWriteBuffer( uint16_t addr, uint8_t *buffer, uint16_t size )
{
    eeprom_write_array(addr, buffer, size);

    return SUCCESS;
}

uint8_t EepromMcuReadBuffer( uint16_t addr, uint8_t *buffer, uint16_t size )
{
    uint8_t len;
    eeprom_read_array(addr, buffer, &len);

    if (len == size)
        return SUCCESS;

    return FAIL;
}

void EepromMcuSetDeviceAddr( uint8_t addr )
{
}

uint8_t EepromMcuGetDeviceAddr( void )
{
    return FAIL;
}
