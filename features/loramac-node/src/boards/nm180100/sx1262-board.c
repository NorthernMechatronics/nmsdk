/******************************************************************************
 * Standard header files
 ******************************************************************************/
#include <stdlib.h>

/******************************************************************************
 * Ambiq Micro header files
 ******************************************************************************/
#include <am_mcu_apollo.h>
#include <am_util.h>

/******************************************************************************
 * FreeRTOS header files
 ******************************************************************************/
#include <FreeRTOSConfig.h>

/******************************************************************************
 * LoRaMac-node header files
 ******************************************************************************/
#include <utilities.h>
#include <radio.h>
#include <sx126x-board.h>

/******************************************************************************
 * Macros
 ******************************************************************************/
#define SX1262_IOM_MODULE                      (3)

#define AM_BSP_GPIO_RADIO_NRESET        44
static const am_hal_gpio_pincfg_t s_AM_BSP_GPIO_RADIO_NRESET =
{
    .uFuncSel            = AM_HAL_PIN_44_GPIO,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA
};

#define AM_BSP_GPIO_RADIO_BUSY          39
static const am_hal_gpio_pincfg_t s_AM_BSP_GPIO_RADIO_BUSY =
{
    .uFuncSel            = AM_HAL_PIN_39_GPIO,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_ENABLE
};

#define AM_BSP_GPIO_RADIO_DIO1          40
static const am_hal_gpio_pincfg_t s_AM_BSP_GPIO_RADIO_DIO1 =
{
    .uFuncSel            = AM_HAL_PIN_40_GPIO,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_ENABLE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI
};

#define AM_BSP_GPIO_RADIO_DIO3          47
static const am_hal_gpio_pincfg_t s_AM_BSP_GPIO_RADIO_DIO3 =
{
    .uFuncSel            = AM_HAL_PIN_47_GPIO,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_ENABLE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI
};

#define AM_BSP_GPIO_RADIO_NSS           36
#define AM_BSP_RADIO_NSS_CHNL           1
static const am_hal_gpio_pincfg_t s_AM_BSP_GPIO_RADIO_NSS =
{
    .uFuncSel            = AM_HAL_PIN_36_NCE36,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .eGPOutcfg           = AM_HAL_GPIO_PIN_OUTCFG_PUSHPULL,
    .eGPInput            = AM_HAL_GPIO_PIN_INPUT_NONE,
    .eIntDir             = AM_HAL_GPIO_PIN_INTDIR_LO2HI,
    .uIOMnum             = 3,
    .uNCE                = 1,
    .eCEpol              = AM_HAL_GPIO_PIN_CEPOL_ACTIVELOW
};

#define AM_BSP_GPIO_RADIO_MISO          43
static const am_hal_gpio_pincfg_t s_AM_BSP_GPIO_RADIO_MISO =
{
    .uFuncSel            = AM_HAL_PIN_43_M3MISO,
    .uIOMnum             = 3
};

#define AM_BSP_GPIO_RADIO_MOSI          38
static const am_hal_gpio_pincfg_t s_AM_BSP_GPIO_RADIO_MOSI =
{
    .uFuncSel            = AM_HAL_PIN_38_M3MOSI,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_2MA,
    .uIOMnum             = 3
};

#define AM_BSP_GPIO_RADIO_CLK           42
static const am_hal_gpio_pincfg_t s_AM_BSP_GPIO_RADIO_CLK =
{
    .uFuncSel            = AM_HAL_PIN_42_M3SCK,
    .eDriveStrength      = AM_HAL_GPIO_PIN_DRIVESTRENGTH_12MA,
    .uIOMnum             = 3
};



/******************************************************************************
 * Local declarations
 ******************************************************************************/
static am_hal_iom_config_t sx1262_spi_cfg;
static void *sx1262_spi_handle;
static am_hal_gpio_handler_t sx1262_isr_chain = NULL;

static RadioOperatingModes_t OperatingMode;

/******************************************************************************
 * Local functions
 ******************************************************************************
 * s_sx1262_spi_write
 * ------------------
 * Ambiq-specific SPI write function
 *
 * Parameters :
 *    @cmd    - Command
 *    @buf    - Command data
 *    @len    - Length of command data
 *
 ******************************************************************************/
static void s_sx1262_spi_write(uint8_t cmd, uint8_t *buf, uint8_t len)
{
  am_hal_iom_transfer_t tx;

  tx.ui32InstrLen = 1;
  tx.ui32Instr = cmd;
  tx.eDirection = AM_HAL_IOM_TX;
  tx.ui32NumBytes = len;
  tx.pui32TxBuffer = (uint32_t *) buf;
  tx.bContinue = false;
  tx.ui8RepeatCount  = 0;
  tx.ui32PauseCondition = 0;
  tx.ui32StatusSetClr = 0;
  tx.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

  am_hal_iom_blocking_transfer(sx1262_spi_handle, &tx);
}

/******************************************************************************
 * s_sx1262_spi_write_registers
 * ----------------------------
 * Ambiq-specific SPI write registers function
 *
 * Parameters :
 *    @addr   - Register address
 *    @buf    - Register data
 *    @len    - Length of register data
 *
 ******************************************************************************/
static void s_sx1262_spi_write_registers(uint16_t addr, const uint8_t *buf, uint8_t len)
{
	am_hal_iom_transfer_t tx;
	uint32_t inst = (RADIO_WRITE_REGISTER << 16) | addr;

  tx.ui32InstrLen = 3;
  tx.ui32Instr = inst;
  tx.eDirection = AM_HAL_IOM_TX;
  tx.ui32NumBytes = len;
  tx.pui32TxBuffer = (uint32_t *) buf;
  tx.bContinue = false;
  tx.ui8RepeatCount  = 0;
  tx.ui32PauseCondition = 0;
  tx.ui32StatusSetClr = 0;
  tx.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

  am_hal_iom_blocking_transfer(sx1262_spi_handle, &tx);
}

/******************************************************************************
 * s_sx1262_spi_write_buffer
 * -------------------------
 * Ambiq-specific SPI write buffer function
 *
 * Parameters :
 *    @ofs    - Offset for buffer write
 *    @buf    - Buffer to write
 *    @len    - Length of buffer
 *
 ******************************************************************************/
static void s_sx1262_spi_write_buffer(uint8_t ofs, const uint8_t *buf, uint8_t len)
{
  am_hal_iom_transfer_t tx;
	uint32_t inst = (RADIO_WRITE_BUFFER << 8) | ofs;

  tx.ui32InstrLen = 2;
  tx.ui32Instr = inst;
  tx.eDirection = AM_HAL_IOM_TX;
  tx.ui32NumBytes = len;
  tx.pui32TxBuffer = (uint32_t *) buf;
  tx.bContinue = false;
  tx.ui8RepeatCount  = 0;
  tx.ui32PauseCondition = 0;
  tx.ui32StatusSetClr = 0;
  tx.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

  am_hal_iom_blocking_transfer(sx1262_spi_handle, &tx);
}

/******************************************************************************
 * s_sx1262_spi_read
 * -----------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 ******************************************************************************/
static void s_sx1262_spi_read(uint8_t cmd, uint8_t *buf, uint8_t len)
{
  am_hal_iom_transfer_t rx;

  rx.ui32InstrLen = 1;
  rx.ui32Instr = cmd;
  rx.eDirection = AM_HAL_IOM_RX;
  rx.ui32NumBytes = len;
  rx.pui32RxBuffer = (uint32_t *) buf;
  rx.bContinue = false;
  rx.ui8RepeatCount  = 0;
  rx.ui32PauseCondition = 0;
  rx.ui32StatusSetClr = 0;
  rx.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;
  am_hal_iom_blocking_transfer(sx1262_spi_handle, &rx);
}

/******************************************************************************
 * s_sx1262_spi_read_registers
 * ---------------------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
static void s_sx1262_spi_read_registers(uint16_t addr, uint8_t *buf, uint8_t len)
{
	uint32_t inst = (__builtin_bswap16(addr) << 8) | RADIO_READ_REGISTER;
	am_hal_iom_transfer_t tx, rx;

  tx.ui32InstrLen = 0;
  tx.ui32Instr = 0;
  tx.eDirection = AM_HAL_IOM_TX;
  tx.ui32NumBytes = 4;
  tx.pui32TxBuffer = (uint32_t *) &inst;
  tx.bContinue = true;
  tx.ui8RepeatCount = 0;
  tx.ui32PauseCondition = 0;
  tx.ui32StatusSetClr = 0;
  tx.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;
  am_hal_iom_blocking_transfer(sx1262_spi_handle, &tx);

  rx.ui32InstrLen = 0;
  rx.ui32Instr = 0;
  rx.eDirection = AM_HAL_IOM_RX;
  rx.ui32NumBytes = len;
  rx.pui32RxBuffer = (uint32_t *) buf;
  rx.bContinue = false;
  rx.ui8RepeatCount = 0;
  rx.ui32PauseCondition = 0;
  rx.ui32StatusSetClr = 0;
  rx.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;
  am_hal_iom_blocking_transfer(sx1262_spi_handle, &rx);
}

/******************************************************************************
 * s_sx1262_spi_read_buffer
 * ------------------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
static void s_sx1262_spi_read_buffer(uint8_t ofs, uint8_t *buf, uint8_t len)
{
  am_hal_iom_transfer_t rx;
  uint32_t inst = (RADIO_READ_BUFFER << 16) | (ofs << 8);

  rx.ui32InstrLen = 3;
  rx.ui32Instr = inst;
  rx.eDirection = AM_HAL_IOM_RX;
  rx.ui32NumBytes = len;
  rx.pui32RxBuffer = (uint32_t *) buf;
  rx.bContinue = false;
  rx.ui8RepeatCount  = 0;
  rx.ui32PauseCondition = 0;
  rx.ui32StatusSetClr = 0;
  rx.uPeerInfo.ui32SpiChipSelect = AM_BSP_RADIO_NSS_CHNL;

  am_hal_iom_blocking_transfer(sx1262_spi_handle, &rx);
}

/******************************************************************************
 * s_sx1262_get_irq_status
 * -----------------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
static uint16_t s_sx1262_get_irq_status(void)
{
  uint8_t buf[2];

  SX126xReadCommand(RADIO_GET_IRQSTATUS, buf, 2);

  return (uint16_t) ((buf[0] << 8) | buf[1]);
}


/******************************************************************************
 * s_sx1262_isr
 * ------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
static void s_sx1262_isr(void)
{
  if(sx1262_isr_chain != NULL)
  {
    sx1262_isr_chain();
  }
}
 
/******************************************************************************
 * SX126xIoInit
 * ------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
void SX126xIoInit(void)
{
  am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_NRESET, s_AM_BSP_GPIO_RADIO_NRESET);
  am_hal_gpio_state_write(AM_BSP_GPIO_RADIO_NRESET, AM_HAL_GPIO_OUTPUT_TRISTATE_DISABLE);
  am_hal_gpio_state_write(AM_BSP_GPIO_RADIO_NRESET, AM_HAL_GPIO_OUTPUT_SET);
  am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_BUSY, s_AM_BSP_GPIO_RADIO_BUSY);

  am_hal_gpio_interrupt_register(AM_BSP_GPIO_RADIO_DIO1, (am_hal_gpio_handler_t) s_sx1262_isr);
  am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_DIO1, s_AM_BSP_GPIO_RADIO_DIO1);

  am_hal_gpio_interrupt_clear(AM_HAL_GPIO_BIT(AM_BSP_GPIO_RADIO_DIO1));

  am_hal_gpio_interrupt_enable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_RADIO_DIO1));
  NVIC_SetPriority(GPIO_IRQn, NVIC_configMAX_SYSCALL_INTERRUPT_PRIORITY);
  NVIC_EnableIRQ(GPIO_IRQn);

  sx1262_spi_cfg.eInterfaceMode = AM_HAL_IOM_SPI_MODE;
  sx1262_spi_cfg.ui32ClockFreq = AM_HAL_IOM_4MHZ;
  sx1262_spi_cfg.eSpiMode = AM_HAL_IOM_SPI_MODE_0;

  am_hal_iom_initialize(SX1262_IOM_MODULE, &sx1262_spi_handle);
  am_hal_iom_power_ctrl(sx1262_spi_handle, AM_HAL_SYSCTRL_WAKE, false);
  am_hal_iom_configure(sx1262_spi_handle, &sx1262_spi_cfg);
  am_bsp_iom_pins_enable(SX1262_IOM_MODULE, AM_HAL_IOM_SPI_MODE);
  am_hal_iom_enable(sx1262_spi_handle);

}

/******************************************************************************
 * SX126xIoIrqInit
 * ---------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
void SX126xIoIrqInit(DioIrqHandler dioIrq)
{
  uint32_t mask;

  mask = am_hal_interrupt_master_disable();
  sx1262_isr_chain = (am_hal_gpio_handler_t) dioIrq;
  am_hal_interrupt_master_set(mask);
}

/******************************************************************************
 * SX126xIoDeInit
 * --------------
 * {Insert description here}
 *
 * Parameters :
 *    @{name} - {description}
 *
 * Returns  :
 *    {Description of return value, if applicable}
 ******************************************************************************/
void SX126xIoDeInit(void)
{
  am_hal_gpio_interrupt_disable(AM_HAL_GPIO_BIT(AM_BSP_GPIO_RADIO_DIO1));
  am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_NRESET, g_AM_HAL_GPIO_DISABLE);
  am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_BUSY, g_AM_HAL_GPIO_DISABLE);
  am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_DIO1, g_AM_HAL_GPIO_DISABLE);

  am_hal_iom_uninitialize(sx1262_spi_handle);
  am_hal_iom_power_ctrl(sx1262_spi_handle, AM_HAL_SYSCTRL_DEEPSLEEP, false);
  am_bsp_iom_pins_disable(SX1262_IOM_MODULE, AM_HAL_IOM_SPI_MODE);
  am_hal_iom_disable(sx1262_spi_handle);
}

void SX126xIoDbgInit(void)
{
}

void SX126xIoTcxoInit(void)
{
}

uint32_t SX126xGetBoardTcxoWakeupTime(void)
{
  return 0;
}

void SX126xReset(void)
{
  am_hal_gpio_state_write(AM_BSP_GPIO_RADIO_NRESET, AM_HAL_GPIO_OUTPUT_CLEAR);
  am_util_delay_us(100);
  am_hal_gpio_state_write(AM_BSP_GPIO_RADIO_NRESET, AM_HAL_GPIO_OUTPUT_SET);
  am_util_delay_us(100);
}

void SX126xWaitOnBusy(void)
{
  uint32_t busy = 1;

  while(busy)
  {
    am_hal_gpio_state_read(AM_BSP_GPIO_RADIO_BUSY, AM_HAL_GPIO_INPUT_READ, &busy);
  }
}

void SX126xWakeup(void)
{
  uint8_t status = 0;

  CRITICAL_SECTION_BEGIN();

  s_sx1262_spi_write(RADIO_GET_STATUS, &status, 1);
  SX126xWaitOnBusy();

  CRITICAL_SECTION_END();
}

void SX126xWriteCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size)
{
  SX126xCheckDeviceReady();

  s_sx1262_spi_write(command, buffer, size);
  if(command != RADIO_SET_SLEEP)
  {
      SX126xWaitOnBusy();
  }
}

uint8_t SX126xReadCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size)
{
  uint8_t buf[8];
  uint8_t i;

  SX126xCheckDeviceReady();
  s_sx1262_spi_read(command, buf, size+1);
  SX126xWaitOnBusy();

  for(i = 0; i < size; i++)
  {
    buffer[i] = buf[i+1];
  }

  return buf[0];
}

void SX126xWriteRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
  SX126xCheckDeviceReady();
  s_sx1262_spi_write_registers(address, buffer, size);
  SX126xWaitOnBusy();
}

void SX126xWriteRegister(uint16_t address, uint8_t value)
{
  SX126xWriteRegisters(address, &value, 1);
}

void SX126xReadRegisters(uint16_t address, uint8_t *buffer, uint16_t size)
{
  SX126xCheckDeviceReady();
  s_sx1262_spi_read_registers(address, buffer, size);
  SX126xWaitOnBusy();
}

uint8_t SX126xReadRegister(uint16_t address)
{
  uint8_t value;

  SX126xReadRegisters(address, &value, 1);

  return value;
}

void SX126xWriteBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
  SX126xCheckDeviceReady();
  s_sx1262_spi_write_buffer(offset, buffer, size);
  SX126xWaitOnBusy();
}

void SX126xReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size)
{
  SX126xCheckDeviceReady();
  s_sx1262_spi_read_buffer(offset, buffer, size);
  SX126xWaitOnBusy();
}

void SX126xSetRfTxPower(int8_t power)
{
  SX126xSetTxParams(power, RADIO_RAMP_40_US);
}

uint8_t SX126xGetDeviceId(void)
{
  return SX1262;
}

void SX126xAntSwOn(void)
{
  uint8_t enable = 1;

  s_sx1262_spi_write(RADIO_SET_RFSWITCHMODE, &enable, 1);
}

void SX126xAntSwOff(void)
{
  uint8_t enable = 0;

  s_sx1262_spi_write(RADIO_SET_RFSWITCHMODE, &enable, 1);
}

bool SX126xCheckRfFrequency(uint32_t frequency)
{
  am_util_debug_printf("SX126xCheckRfFrequency(%u)\r\n", frequency);
  return true;
}

RadioOperatingModes_t SX126xGetOperatingMode(void)
{
    return OperatingMode;
}

void SX126xSetOperatingMode(RadioOperatingModes_t mode)
{
    OperatingMode = mode;
}

void SX126xIoRfSwitchInit(void)
{
    SX126xSetDio2AsRfSwitchCtrl(true);
}
