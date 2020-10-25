/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2020, Northern Mechatronics, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
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
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef __NM_DEVICES_LORA_H__
#define __NM_DEVICES_LORA_H__

#define LORA_RADIO_MAX_PHYSICAL_PACKET 0xFF

typedef enum {
    LORA_RADIO_STATUS_SUCCESS,
    LORA_RADIO_STATUS_FAIL,
    LORA_RADIO_STATUS_INVALID_HANDLE,
    LORA_RADIO_STATUS_IN_USE,
    LORA_RADIO_STATUS_TIMEOUT,
    LORA_RADIO_STATUS_OUT_OF_RANGE,
    LORA_RADIO_STATUS_INVALID_ARG,
    LORA_RADIO_STATUS_INVALID_OPERATION,
    LORA_RADIO_STATUS_HW_ERR,
} lora_radio_status_e;

typedef enum {
    LORA_RADIO_STANDBY,
    LORA_RADIO_SLEEP,
    LORA_RADIO_DEEPSLEEP
} lora_radio_power_state_e;

typedef enum {
    LORA_RADIO_SF5 = 5,
    LORA_RADIO_SF6,
    LORA_RADIO_SF7,
    LORA_RADIO_SF8,
    LORA_RADIO_SF9,
    LORA_RADIO_SF10,
    LORA_RADIO_SF11,
    LORA_RADIO_SF12
} lora_radio_spreading_factor_e;

typedef enum {
    LORA_RADIO_BW_7,
    LORA_RADIO_BW_10,
    LORA_RADIO_BW_15,
    LORA_RADIO_BW_20,
    LORA_RADIO_BW_31,
    LORA_RADIO_BW_41,
    LORA_RADIO_BW_62,
    LORA_RADIO_BW_125,
    LORA_RADIO_BW_250,
    LORA_RADIO_BW_500
} lora_radio_bandwidth_e;

typedef enum {
    LORA_CR_4_5 = 0,
    LORA_CR_4_6,
    LORA_CR_4_7,
    LORA_CR_4_8
} lora_radio_coding_rate_e;

typedef enum {
    LORA_RADIO_TXDONE = 0x0001,
    LORA_RADIO_RXDONE = 0x0002,
    LORA_RADIO_PREAMBLEDETECTED = 0x0004,
    LORA_RADIO_SYNCWORDVALID = 0x0008,
    LORA_RADIO_HEADERVALID = 0x0010,
    LORA_RADIO_HEADERERR = 0x0020,
    LORA_RADIO_CRCERR = 0x0040,
    LORA_RADIO_CADDONE = 0x0080,
    LORA_RADIO_CADDETECTED = 0x0100,
    LORA_RADIO_TIMEOUT = 0x0200,
    LORA_RADIO_IRQ_ALL = 0x03FF
} lora_radio_irq_e;

typedef void (*lora_radio_callback_t)(void *);

typedef struct {
    lora_radio_irq_e eIRQ;
    lora_radio_callback_t pfnCallback;
} lora_radio_irq_handler_t;

typedef enum {
    LORA_RADIO_LDR_OPT_OFF = 0,
    LORA_RADIO_LDR_OPT_ON
} lora_radio_low_data_rate_optimization_e;

typedef struct {
    lora_radio_spreading_factor_e eSpreadingFactor;
    lora_radio_bandwidth_e eBandwidth;
    lora_radio_coding_rate_e eCodingRate;
    lora_radio_low_data_rate_optimization_e eLowDataRateOptimization;
} lora_radio_modulation_t;

typedef enum {
    LORA_RADIO_PACKET_LENGTH_VARIABLE = 0,
    LORA_RADIO_PACKET_LENGTH_FIXED
} lora_radio_packet_length_e;

typedef enum { LORA_RADIO_CRC_OFF = 0, LORA_RADIO_CRC_ON } lora_radio_crc_e;

typedef enum {
    LORA_RADIO_IQ_STANDARD = 0,
    LORA_RADIO_IQ_INVERTED
} lora_radio_iq_e;

typedef struct {
    uint16_t ui16PreambleLength;
    lora_radio_packet_length_e ePacketLength;
    uint8_t ui8PayloadLength;
    lora_radio_crc_e eCRC;
    lora_radio_iq_e eIQ;
} lora_radio_packet_t;

typedef enum {
    LORA_RADIO_TX,
    LORA_RADIO_RX,
    LORA_RADIO_TXCARRIER
} lora_radio_mode_e;

typedef struct {
    uint32_t ui32SyncWord;
    uint32_t ui32Frequency;
    uint32_t ui32Timeout; // Transmit: timeout in ms
                          // Receive:
                          //   [7:0]  number of symbols required for lock
                          //   [31:8] delay in ms before reception, set to
                          //          0 for continuous reception
    lora_radio_modulation_t *psModulationParameters;
    lora_radio_packet_t *psPacketParameters;
    uint8_t *pui8Payload;
    uint8_t ui8Power;
    lora_radio_mode_e eMode;
} lora_radio_transfer_t;

typedef struct {
    uint8_t *pui8Payload;
    uint8_t ui8PayloadLength;
    int8_t i8Rssi;
    int8_t i8Snr;
    int8_t i8Rscp;
} lora_radio_physical_packet_t;

typedef struct {
    uint32_t ui32Parameter;
    uint8_t *pui8Content;
} lora_radio_config_t;

extern uint32_t lora_radio_initialize(void **ppHandle);
extern uint32_t lora_radio_deinitialize(void *pHandle);
extern uint32_t lora_radio_reset(void *pHandle);
extern uint32_t lora_radio_power_ctrl(void *pHandle,
                                      lora_radio_power_state_e ePowerState);
extern uint32_t lora_radio_config(void *pHandle, lora_radio_config_t *psConfig);
extern uint32_t lora_radio_transfer(void *pHandle,
                                    lora_radio_transfer_t *psTransaction);
extern void lora_radio_callback_list_init();
extern void lora_radio_callback_list_deinit();
extern uint32_t lora_radio_callback_register(lora_radio_irq_e eIrq,
                                             lora_radio_callback_t pfnCallback);
extern uint32_t
lora_radio_callback_deregister(lora_radio_irq_e eIrq,
                               lora_radio_callback_t pfnCallback);

#endif /* __NM_DEVICES_LORA_H__ */
