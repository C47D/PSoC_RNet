/****************************************************************************
* This file is part of the nRF24 Custom Component for PSoC Devices.
*
* Copyright (C) 2017 Carlos Diaz <carlos.santiago.diaz@gmail.com>
*
* Permission to use, copy, modify, and/or distribute this software for any
* purpose with or without fee is hereby granted, provided that the above
* copyright notice and this permission notice appear in all copies.
*
* THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
* WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
* MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
* ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
* WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
* ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
* OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
****************************************************************************/

#ifndef RF1_LL_SPI_H
#define RF1_LL_SPI_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#include "RF1_REGS.h"

uint8_t RF1_read_register(const nrf_register reg);
void RF1_read_long_register(const nrf_register reg, uint8_t* data , const size_t size);
void RF1_write_register(const nrf_register reg, const uint8_t data);
void RF1_write_long_register(const nrf_register reg, const uint8_t* data, const size_t size);
bool RF1_read_bit(const nrf_register reg, const uint8_t bit);
void RF1_clear_bit(const nrf_register reg, const uint8_t bit);
void RF1_set_bit(const nrf_register reg, const uint8_t bit);

#endif /* RF1_LL_SPI_H */

/* [] END OF FILE */
