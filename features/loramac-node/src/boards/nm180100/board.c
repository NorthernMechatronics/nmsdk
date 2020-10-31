#include <am_mcu_apollo.h>
#include <am_util.h>

#include "board.h"


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
}

void BoardInitMcu(void)
{
	SX126xIoInit();
}

void BoardResetMcu(void)
{
	CRITICAL_SECTION_BEGIN();

	Reset_Handler();
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
void BoardGetUniqueId(uint8_t *id)
{
  am_util_id_t i;

  am_util_id_device(&i);

  id[0] = 0xCC;
  id[1] = 0x5D;
  id[2] = 0x78;
  id[3] = 0xFF;
  id[4] = (uint8_t) (i.sMcuCtrlDevice.ui32ChipID0);
  id[5] = (uint8_t) (i.sMcuCtrlDevice.ui32ChipID0 >> 8);
  id[6] = (uint8_t) (i.sMcuCtrlDevice.ui32ChipID0 >> 16);
  id[7] = (uint8_t) (i.sMcuCtrlDevice.ui32ChipID0 >> 24);
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
