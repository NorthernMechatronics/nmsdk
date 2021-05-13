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
#include <am_mcu_apollo.h>
#include "utilities.h"
#include "eeprom-board.h"

uint8_t EepromMcuWriteBuffer( uint16_t addr, uint8_t *buffer, uint16_t size )
{
    eeprom_write_array_len(addr + 1, buffer, size);
    return 1;
}

uint8_t EepromMcuReadBuffer( uint16_t addr, uint8_t *buffer, uint16_t size )
{
    if (!eeprom_read_array_len(addr + 1, buffer, size))
    {
        return 0;
    }

    return 1;
}

void EepromMcuSetDeviceAddr( uint8_t addr )
{
}

uint8_t EepromMcuGetDeviceAddr( void )
{
    return 0;
}
