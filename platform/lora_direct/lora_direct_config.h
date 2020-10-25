/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2020, Northern Mechatronics, Inc.
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
#ifndef _LORA_DIRECT_CONFIG_H_
#define _LORA_DIRECT_CONFIG_H_

#if defined(__cplusplus)
extern "C" {
#endif // defined(__cplusplus)

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
#endif // defined(__cplusplus)

#endif /* _LORA_DIRECT_CONFIG_H_ */
