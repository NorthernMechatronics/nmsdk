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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <nm_devices_lora.h>

const char LORA_RADIO_FREQUENCY[] = "freq";
const char LORA_RADIO_POWER[] = "power";
const char LORA_RADIO_SPREADING_FACTOR[] = "sf";
const char LORA_RADIO_BANDWIDTH[] = "bw";
const char LORA_RADIO_CODING_RATE[] = "cr";
const char LORA_RADIO_SYNCWORD[] = "sync";

uint32_t lora_radio_frequency = 915000000;
uint32_t lora_radio_power = 22;
uint32_t lora_radio_syncword = 0x3444;
uint32_t lora_radio_transmit_period = 2;

lora_radio_modulation_t gsLoRaModulationParameter;

lora_radio_packet_t gsLoRaPacketParameter;

static const lora_radio_modulation_t defaultLoRaModulationParameter = {
    .eSpreadingFactor = LORA_RADIO_SF10,
    .eBandwidth = LORA_RADIO_BW_125,
    .eCodingRate = LORA_CR_4_5,
    .eLowDataRateOptimization = LORA_RADIO_LDR_OPT_OFF};

static const lora_radio_packet_t defaultLoRaPacketParameter = {
    .ui16PreambleLength = 0x0008,
    .ePacketLength = LORA_RADIO_PACKET_LENGTH_VARIABLE,
    .ui8PayloadLength = LORA_RADIO_MAX_PHYSICAL_PACKET,
    .eCRC = LORA_RADIO_CRC_OFF,
    .eIQ = LORA_RADIO_IQ_INVERTED};

/*
 * Application must implement the following:
 *
void lora_direct_radio_configuration_reset(void)
{
	memcpy(&gsLoRaModulationParameter, &defaultLoRaModulationParameter, sizeof(lora_radio_modulation_t));
	memcpy(&gsLoRaPacketParameter, &defaultLoRaPacketParameter, sizeof(lora_radio_packet_t));
}
*/
