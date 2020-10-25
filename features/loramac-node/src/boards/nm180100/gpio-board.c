#include <am_mcu_apollo.h>
#include <FreeRTOSConfig.h>

#include "gpio-board.h"

#define AM_BSP_GPIO_RADIO_BUSY   39
#define AM_BSP_GPIO_RADIO_NRESET 44

void GpioMcuInit(Gpio_t *obj, PinNames pin, PinModes mode, PinConfigs config, PinTypes type, uint32_t value)
{
  obj->pin = pin;

  if( pin == NC )
  {
    return;
  }

  if(mode == PIN_INPUT)
  {
    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_BUSY, g_AM_HAL_GPIO_INPUT);
  }
  else
  {
    am_hal_gpio_pinconfig(AM_BSP_GPIO_RADIO_NRESET, g_AM_HAL_GPIO_OUTPUT);
    am_hal_gpio_state_write(AM_BSP_GPIO_RADIO_NRESET, AM_HAL_GPIO_OUTPUT_TRISTATE_DISABLE);
    am_hal_gpio_state_write(AM_BSP_GPIO_RADIO_NRESET, AM_HAL_GPIO_OUTPUT_SET);
  }
}

void GpioMcuSetContext(Gpio_t *obj, void* context)
{
  obj->Context = context;
}

void GpioMcuSetInterrupt(Gpio_t *obj, IrqModes irqMode, IrqPriorities irqPriority, GpioIrqHandler *irqHandler)
{
  am_hal_gpio_interrupt_clear(AM_HAL_GPIO_BIT(obj->pin));
  am_hal_gpio_interrupt_register(obj->pin, (am_hal_gpio_handler_t) irqHandler);
  NVIC_EnableIRQ(GPIO_IRQn);
  NVIC_SetPriority(GPIO_IRQn, NVIC_configMAX_SYSCALL_INTERRUPT_PRIORITY);
}

void GpioMcuRemoveInterrupt(Gpio_t *obj)
{
  am_hal_gpio_interrupt_disable(AM_HAL_GPIO_BIT(obj->pin));
}

void GpioMcuWrite(Gpio_t *obj, uint32_t value)
{
  if(obj == NULL)
  {
      while(1);
  }
  if(obj->pin == NC)
  {
      return;
  }
  am_hal_gpio_state_write(obj->pin, value ? AM_HAL_GPIO_OUTPUT_SET : AM_HAL_GPIO_OUTPUT_CLEAR);
}

void GpioMcuToggle(Gpio_t *obj)
{
  uint32_t value;

  if(obj == NULL)
  {
    while( 1 );
  }
  if(obj->pin == NC)
  {
    return;
  }
  am_hal_gpio_state_read(obj->pin, AM_HAL_GPIO_INPUT_READ, &value);
  am_hal_gpio_state_write(obj->pin, value ? AM_HAL_GPIO_OUTPUT_CLEAR : AM_HAL_GPIO_OUTPUT_SET);
}

uint32_t GpioMcuRead(Gpio_t *obj)
{
  uint32_t value;

  if(obj == NULL)
  {
    while(1);
  }
  if(obj->pin == NC)
  {
    return 0;
  }
  am_hal_gpio_state_read(obj->pin, AM_HAL_GPIO_INPUT_READ, &value);

  return value;
}
