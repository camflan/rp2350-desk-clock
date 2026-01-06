// src/hal/pio_qspi.h
#ifndef PIO_QSPI_H
#define PIO_QSPI_H

#include <stdint.h>
#include <stddef.h>
#include "pico/types.h"

// Initialize PIO QSPI interface
void pio_qspi_init(uint clk_pin, uint d0_pin, uint baudrate);

// Low-level write functions - caller must manage CS pin
void pio_qspi_1bit_write_blocking(uint8_t *buf, size_t len);
void pio_qspi_4bit_write_blocking(uint8_t *buf, size_t len);

#endif
