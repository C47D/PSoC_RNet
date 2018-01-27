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

/**
 * @file RF1_FUNCS.h
 *
 * @brief This file define all the functions available to the user.
 */

#ifndef RF1_FUNCS_H
#define RF1_FUNCS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "RF1_REGS.h"

// PSoC Component Functions
void RF1_start(void);
void RF1_init(void);
void RF1_enable(void);
void RF1_stop(void);
void RF1_sleep(void);
void RF1_wakeup(void);
void RF1_save_config(void);
void RF1_restore_config(void);

// nRF24 Functions

// Configuration functions
void RF1_set_mode(const nrf_mode mode);
void RF1_set_power_down_mode(void);
void RF1_set_standby_i_mode(void);
void RF1_set_standby_ii_mode(void);
void RF1_set_rx_mode(void);
void RF1_set_tx_mode(void);

// Functions related to registers

// CONFIG Reg
// EN_AA_Reg
void RF1_enable_auto_ack(const nrf_pipe pipe);
void RF1_disable_auto_ack(const nrf_pipe pipe);
// EN_RXADDR Reg
// SETUP_AW Reg
// SETUP_RETR Reg
// RF_CH Reg
void RF1_set_channel(uint8_t channel);
// RF_SETUP Reg
// STATUS Reg

void RF1_set_address_width(const nrf_setup_address_width addr_width);
uint8_t RF1_get_address_width(void);
// RX ADDR P0 Reg
void RF1_set_rx_pipe_0_address(const uint8_t *addr, size_t size);
void RF1_get_rx_pipe_0_address(uint8_t *addr, size_t size);
// RX ADDR P1 Reg
void RF1_set_rx_pipe_1_address(const uint8_t *addr, size_t size);
void RF1_get_rx_pipe_1_address(uint8_t *addr, size_t size);
// RX ADDR P2 Reg
void RF1_set_rx_pipe_2_address(const uint8_t addr_lsb);
void RF1_get_rx_pipe_2_address(uint8_t *addr, size_t size);
// RX ADDR P3 Reg
void RF1_set_rx_pipe_3_address(const uint8_t addr_lsb);
void RF1_get_rx_pipe_3_address(uint8_t *addr, size_t size);
// RX ADDR P4 Reg
void RF1_set_rx_pipe_4_address(const uint8_t addr_lsb);
void RF1_get_rx_pipe_4_address(uint8_t *addr, size_t size);
// RX ADDR P5 Reg
void RF1_set_rx_pipe_5_address(const uint8_t addr_lsb);
void RF1_get_rx_pipe_5_address(uint8_t *addr, size_t size);
// TX_ADDR Reg
void RF1_set_tx_address(const uint8_t *const addr, size_t size);
void RF1_get_tx_address(uint8_t *addr, size_t size);

// RX_PW_P0, RX_PW_P1, RX_PW_P2, RX_PW_P3, RX_PW_P4, RX_PW_P5 Reg
void RF1_set_payload_size(const nrf_pipe_payload_size pipe,
                                          uint8_t size);
uint8_t RF1_get_payload_size(const nrf_pipe_payload_size pipe);

// FIFO_STATUS Reg
void RF1_reuse_last_transmitted_payload(void);
// DYNPD Reg
void RF1_enable_dynamic_payload(const nrf_pipe pipe);
void RF1_disable_dynamic_payload(const nrf_pipe pipe);
// FEATURE Reg
void RF1_enable_dynamic_payload_length(void);
void RF1_enable_payload_with_ack(void);
void RF1_enable_payload_with_no_ack_cmd(void);
void RF1_disable_dynamic_payload_length(void);
void RF1_disable_payload_with_ack(void);
void RF1_disable_payload_with_no_ack_cmd(void);

/* General purpose functions */
void RF1_start_listening(void);
void RF1_stop_listening(void);
void RF1_transmit_pulse(void);
uint8_t RF1_get_status(void);
uint8_t RF1_get_retransmissions_count(void);
uint8_t RF1_get_lost_packets_count(void);
void RF1_put_in_tx_fifo(const uint8_t *data, size_t size);
void RF1_transmit(const uint8_t *data, size_t size);
bool RF1_is_data_ready(void);
void RF1_get_rx_payload(uint8_t *payload, const size_t size);
void RF1_tx_transmit_wait_no_ack(const uint8_t *data, size_t size);
void RF1_rx_write_payload(const nrf_pipe pipe, const uint8_t *data,
                                         size_t size);
uint8_t RF1_get_data_pipe_with_payload(void);
uint8_t RF1_received_power_detector(void);
bool RF1_is_tx_fifo_full(void);
bool RF1_is_rx_fifo_empty(void);
bool RF1_test_carrier(void);

// IRQ Handle functions
void RF1_clear_all_irqs(void);
void RF1_clear_irq_flag(const nrf_irq irq_flag);
nrf_irq RF1_get_irq_flag(void);
void RF1_poll_interrupt(void);
uint8_t RF1_get_status_clear_irq(void);

// command wrappers
void RF1_flush_rx(void);
void RF1_flush_tx(void);

#endif /* RF1_FUNCS_H */

/* [] END OF FILE */
