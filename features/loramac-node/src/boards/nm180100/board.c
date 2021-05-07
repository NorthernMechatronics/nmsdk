#include <am_mcu_apollo.h>
#include <am_util.h>

#include <sx126x-board.h>
#include "board.h"
#include "rtc-board.h"
#include "eeprom_emulation_conf.h"
#include "eeprom_emulation.h"

/******************************************************************************
 * Global functions
 ******************************************************************************
 * BoardCriticalSectionBegin
 * -------------------------
 * {Insert description here}
 *
 * Parameters :
 *  @mask     - {description}
 *
 ******************************************************************************/
void BoardCriticalSectionBegin(uint32_t *mask)
{
  *mask = am_hal_interrupt_master_disable();
}

void BoardCriticalSectionEnd(uint32_t *mask)
{
  am_hal_interrupt_master_set(*mask);
}

void BoardInitPeriph(void)
{
	RtcInit();
    if (!eeprom_init(EEPROM_EMULATION_FLASH_PAGES))
    {
        eeprom_format(EEPROM_EMULATION_FLASH_PAGES);
    }
}

void BoardInitMcu(void)
{
	SX126xIoInit();
}

void BoardResetMcu(void)
{
	CRITICAL_SECTION_BEGIN();
    NVIC_SystemReset();
}

void BoardDeInitMcu(void)
{
	SX126xIoDeInit();
}

uint32_t BoardGetRandomSeed(void)
{
  am_util_id_t i;
  uint32_t ret;

  am_util_id_device(&i);
  ret = ~(i.sMcuCtrlDevice.ui32ChipID0);
  ret ^= i.sMcuCtrlDevice.ui32ChipID1;
  ret ^= i.sMcuCtrlDevice.ui32ChipRev << 3;
  ret ^= i.sMcuCtrlDevice.ui32VendorID >> 3;
  ret ^= i.sMcuCtrlDevice.ui32SKU << 6;
  ret ^= i.sMcuCtrlDevice.ui32Qualified >> 6;

  return ret;
}

uint16_t BoardBatteryMeasureVolage(void)
{
  return 0;
}

uint32_t BoardGetBatteryVoltage(void)
{
  return 0;
}

uint8_t BoardGetBatteryLevel(void)
{
  return 0;
}

uint8_t GetBoardPowerSource(void)
{
  return USB_POWER;
}

void LpmEnterStopMode(void)
{
}

void LpmExitStopMode(void)
{
}

void LpmEnterSleepMode(void)
{
}

void BoardLowPowerHandler(void)
{
}
