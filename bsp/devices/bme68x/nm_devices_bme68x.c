#include <stdint.h>
#include <stdlib.h>

#include <am_mcu_apollo.h>
#include <am_util.h>
#include <am_bsp.h>

#include "bme68x.h"
#include "nm_devices_bme68x.h"

#define BME68X_IOM_MODULE  0

static void    *pBME68xIOMHandle;
static uint8_t  dev_addr;

am_hal_iom_config_t gBME68xIOMConfig =
{
    .eInterfaceMode     = AM_HAL_IOM_I2C_MODE,
    .ui32ClockFreq      = AM_HAL_IOM_1MHZ,
    .eSpiMode           = AM_HAL_IOM_SPI_MODE_0,
    .ui32NBTxnBufLength = 0,
    .pNBTxnBuf = NULL,
};

BME68X_INTF_RET_TYPE bme68x_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *(uint8_t*)intf_ptr;

    am_hal_iom_transfer_t  Transaction;

    Transaction.uPeerInfo.ui32I2CDevAddr = dev_addr;
    Transaction.ui8Priority = 1;
    Transaction.eDirection = AM_HAL_IOM_RX;
    Transaction.ui32InstrLen = 1;
    Transaction.ui32Instr = reg_addr;
    Transaction.ui32NumBytes = len;
    Transaction.pui32RxBuffer = (uint32_t *)reg_data;
    Transaction.ui8RepeatCount = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.bContinue = false;

    return am_hal_iom_blocking_transfer(pBME68xIOMHandle, &Transaction);
}

BME68X_INTF_RET_TYPE bme68x_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    uint8_t dev_addr = *(uint8_t*)intf_ptr;

    am_hal_iom_transfer_t  Transaction;

    Transaction.uPeerInfo.ui32I2CDevAddr = dev_addr;
    Transaction.ui8Priority = 1;
    Transaction.eDirection = AM_HAL_IOM_TX;
    Transaction.ui32InstrLen = 1;
    Transaction.ui32Instr = reg_addr;
    Transaction.ui32NumBytes = len;
    Transaction.pui32TxBuffer = (uint32_t *)reg_data;
    Transaction.ui8RepeatCount = 0;
    Transaction.ui32PauseCondition = 0;
    Transaction.ui32StatusSetClr = 0;
    Transaction.bContinue = false;

    return am_hal_iom_blocking_transfer(pBME68xIOMHandle, &Transaction);
}

BME68X_INTF_RET_TYPE bme68x_spi_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    return BME68X_E_COM_FAIL;
}

BME68X_INTF_RET_TYPE bme68x_spi_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr)
{
    return BME68X_E_COM_FAIL;
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
            dev_addr = BME68X_I2C_ADDR_LOW;
            bme->read = bme68x_i2c_read;
            bme->write = bme68x_i2c_write;
            bme->intf = BME68X_I2C_INTF;

            am_hal_iom_initialize(BME68X_IOM_MODULE, &pBME68xIOMHandle);
            am_hal_iom_power_ctrl(pBME68xIOMHandle, AM_HAL_SYSCTRL_WAKE, false);
            am_hal_iom_configure(pBME68xIOMHandle, &gBME68xIOMConfig);
            am_hal_iom_enable(pBME68xIOMHandle);

            am_bsp_iom_pins_enable(BME68X_IOM_MODULE, AM_HAL_IOM_I2C_MODE);
        }
        else if (intf == BME68X_SPI_INTF)
        {
            am_util_stdio_printf("SPI Interface is not supported\n");
            return BME68X_E_COM_FAIL;
        }
        bme->delay_us = bme68x_delay_us;
        bme->intf_ptr = &dev_addr;
        /* The ambient temperature in deg C is used for defining the heater temperature */
        bme->amb_temp = 25;
    }
    else
    {
        rslt = BME68X_E_NULL_PTR;
    }

    return rslt;
}

void bme68x_coines_deinit(void)
{
    am_bsp_iom_pins_disable(BME68X_IOM_MODULE, AM_HAL_IOM_I2C_MODE);
    am_hal_iom_disable(pBME68xIOMHandle);
    am_hal_iom_power_ctrl(pBME68xIOMHandle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
    am_hal_iom_uninitialize(pBME68xIOMHandle);
}
