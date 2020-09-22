/*
 * lora_config.h
 *
 *  Created on: Oct 24, 2019
 *      Author: joshua
 */

#ifndef _LORA_CONFIG_H_
#define _LORA_CONFIG_H_

#if defined(__cplusplus)
extern "C" {
#endif  // defined(__cplusplus)

// constant strings used by lora_console to set the radio configuration
extern const char LORA_RADIO_FREQUENCY[];
extern const char LORA_RADIO_POWER[];
extern const char LORA_RADIO_SPREADING_FACTOR[];
extern const char LORA_RADIO_BANDWIDTH[];
extern const char LORA_RADIO_CODING_RATE[];
extern const char LORA_RADIO_SYNCWORD[];

extern uint32_t lora_radio_frequency;
extern uint32_t lora_radio_power;
extern uint32_t lora_radio_syncword;
extern uint32_t lora_radio_transmit_period;

extern lora_radio_modulation_t gsLoRaModulationParameter;
extern lora_radio_packet_t gsLoRaPacketParameter;

extern void lora_direct_radio_configuration_reset(void);

#if defined(__cplusplus)
}
#endif  // defined(__cplusplus)

#endif /* _LORA_CONFIG_H_ */
