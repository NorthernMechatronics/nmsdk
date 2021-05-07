/*
 * BSD 3-Clause License
 *
 * Copyright (c) 2021, Northern Mechatronics, Inc.
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
#ifndef _EEPROM_EMULATION_H_
#define _EEPROM_EMULATION_H_

#ifdef __cplusplus
extern "C" {
#endif

bool eeprom_init(uint32_t);
bool eeprom_format(uint32_t);
bool eeprom_read(uint16_t address, uint16_t *data);
bool eeprom_read_array(uint16_t address, uint8_t *data, uint8_t *len);
bool eeprom_read_array_len(uint16_t address, uint8_t *data, uint16_t size);
void eeprom_write(uint16_t address, uint16_t data);
void eeprom_write_array(uint16_t address, uint8_t *data, uint8_t len);
void eeprom_write_array_len(uint16_t address, uint8_t *data, uint16_t size);
bool eeprom_delete(uint16_t virtual_address);
bool eeprom_delete_array(uint16_t virtual_address);

uint32_t eeprom_erase_counter(void);

#ifdef __cplusplus
}
#endif

#endif /* _EEPROM_EMULATION_H_ */
