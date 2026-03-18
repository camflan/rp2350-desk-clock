#ifndef PIO_QSPI_H
#define PIO_QSPI_H

#include <stddef.h>
#include <stdint.h>
#include "pico/types.h"

void pio_qspi_init(uint clk_pin, uint d0_pin, uint baudrate);
void pio_qspi_1bit_write_blocking(const uint8_t *buf, size_t len);
void pio_qspi_4bit_write_blocking(const uint8_t *buf, size_t len);

#endif
