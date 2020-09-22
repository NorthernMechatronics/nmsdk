/*
 * lora_config.c
 *
 *  Created on: Oct 24, 2019
 *      Author: joshua
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#include <nm_devices_lora.h>

const char LORA_RADIO_FREQUENCY[] = "freq";
const char LORA_RADIO_POWER[] = "power";
const char LORA_RADIO_SPREADING_FACTOR[] = "sf";
const char LORA_RADIO_BANDWIDTH[] = "bw";
const char LORA_RADIO_CODING_RATE[] = "cr";
const char LORA_RADIO_SYNCWORD[] = "sync";

uint32_t lora_radio_frequency = 915000000;
uint32_t lora_radio_power     = 22;
uint32_t lora_radio_syncword  = 0x3444;
uint32_t lora_radio_transmit_period = 2;

lora_radio_modulation_t gsLoRaModulationParameter;

lora_radio_packet_t gsLoRaPacketParameter;

static const lora_radio_modulation_t defaultLoRaModulationParameter =
{
	.eSpreadingFactor = LORA_RADIO_SF10,
	.eBandwidth       = LORA_RADIO_BW_125,
	.eCodingRate      = LORA_CR_4_5,
	.eLowDataRateOptimization = LORA_RADIO_LDR_OPT_OFF
};

static const lora_radio_packet_t defaultLoRaPacketParameter =
{
	.ui16PreambleLength = 0x0008,
	.ePacketLength = LORA_RADIO_PACKET_LENGTH_VARIABLE,
	.ui8PayloadLength = LORA_RADIO_MAX_PHYSICAL_PACKET,
	.eCRC          = LORA_RADIO_CRC_OFF,
	.eIQ           = LORA_RADIO_IQ_INVERTED
};

/*
 * Application must implement the following:
 *
void lora_direct_radio_configuration_reset(void)
{
	memcpy(&gsLoRaModulationParameter, &defaultLoRaModulationParameter, sizeof(lora_radio_modulation_t));
	memcpy(&gsLoRaPacketParameter, &defaultLoRaPacketParameter, sizeof(lora_radio_packet_t));
}
*/
