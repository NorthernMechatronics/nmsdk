#include <stdint.h>
#include <stdlib.h>

#include <am_mcu_apollo.h>
#include <am_util.h>
#include <am_bsp.h>

#include "bme68x.h"
#include "nm_devices_bme68x.h"

static uint8_t dev_addr;

BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *(uint8_t*)intf_ptr;

    return BME68X_OK;
}

BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *(uint8_t*)intf_ptr;

    return BME68X_OK;
}

BME68X_INTF_RET_TYPE bme68x_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *(uint8_t*)intf_ptr;

    return BME68X_OK;
}

BME68X_INTF_RET_TYPE bme68x_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *(uint8_t*)intf_ptr;

    return BME68X_OK;
}

void bme68x_delay_us(uint32_t period, void *intf_ptr)
{
    am_util_delay_us(period);
}

void bme68x_check_rslt(const char api_name[], int8_t rslt)
{
    switch (rslt)
    {
        case BME68X_OK:
            break;
        case BME68X_E_NULL_PTR:
            am_util_stdio_printf("Error [%d] : Null pointer\r\n", rslt);
            break;
        case BME68X_E_COM_FAIL:
            am_util_stdio_printf("Error [%d] : Communication failure\r\n", rslt);
            break;
        case BME68X_E_INVALID_LENGTH:
            am_util_stdio_printf("Error [%d] : Incorrect length parameter\r\n", rslt);
            break;
        case BME68X_E_DEV_NOT_FOUND:
            am_util_stdio_printf("Error [%d] : Device not found\r\n", rslt);
            break;
        default:
            am_util_stdio_printf("Error [%d] : Unknown error code\r\n", rslt);
            break;
    }
}

int8_t bme68x_interface_init(struct bme68x_dev *bme, uint8_t intf)
{
    int8_t rslt = BME68X_OK;

    if (bme != NULL)
    {
        if (intf == BME68X_I2C_INTF)
        {
            am_util_stdio_printf("I2C Interface\n");
            dev_addr = BME68X_I2C_ADDR_LOW;
            bme->read = bme68x_i2c_read;
            bme->write = bme68x_i2c_write;
            bme->intf = BME68X_I2C_INTF;
        }
        else if (intf == BME68X_SPI_INTF)
        {
            am_util_stdio_printf("SPI Interface\n");
            dev_addr = 0x00;
            bme->read = bme68x_spi_read;
            bme->write = bme68x_spi_write;
            bme->intf = BME68X_SPI_INTF;
        }
        bme->delay_us = bme68x_delay_us;
        bme->intf_ptr = &dev_addr;
        bme->amb_temp = 25; /* The ambient temperature in deg C is used for defining the heater temperature */
    }
    else
    {
        rslt = BME68X_E_NULL_PTR;
    }

    return rslt;
}

void bme68x_coines_deinit(void)
{
}
