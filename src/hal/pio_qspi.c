// src/hal/pio_qspi.c
// Based on jblanked/Waveshare implementation
#include "pio_qspi.h"
#include "pio_qspi.pio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include <stdio.h>

static PIO pio = pio0;
static uint sm = 0;

// Helper to drain the PIO state machine
static inline void pio_sm_drain_tx_fifo_inline(PIO pio, uint sm) {
    // Wait for TX FIFO to be empty
    while (!pio_sm_is_tx_fifo_empty(pio, sm))
        tight_loop_contents();

    // Wait for state machine to finish shifting out all data
    // This is critical - FIFO empty doesn't mean transmission complete!
    uint32_t stall_mask = 1u << (PIO_FDEBUG_TXSTALL_LSB + sm);
    pio->fdebug = stall_mask;
    while (!(pio->fdebug & stall_mask))
        tight_loop_contents();
}

void pio_qspi_init(uint clk_pin, uint d0_pin, uint baudrate) {
    printf("Initializing PIO QSPI…\n");
    printf("  PIO: pio0\n");
    printf("  State machine: %d\n", sm);
    printf("  CLK pin: GPIO%d\n", clk_pin);
    printf("  D0-D3 pins: GPIO%d-GPIO%d\n", d0_pin, d0_pin + 3);

    // Calculate clock divider
    float sys_clk = clock_get_hz(clk_sys);
    float div = sys_clk / (baudrate * 2.0f);
    if (div < 1.0f)
        div = 1.0f;

    printf("  System clock: %.0f Hz\n", sys_clk);
    printf("  Target baudrate: %d Hz\n", baudrate);
    printf("  Clock divider: %.2f\n", div);
    printf("  Actual baudrate: %.0f Hz\n", sys_clk / (div * 2.0f));

    // Load the PIO program
    uint offset = pio_add_program(pio, &qspi_program);
    printf("  PIO program offset: %d\n", offset);

    // Initialize the state machine
    qspi_program_init(pio, sm, offset, clk_pin, d0_pin, div);

    printf("PIO QSPI initialized\n");
}

// 1-bit mode: Convert each byte to 4 nibbles for SPI compatibility
// Used for commands and command parameters
void pio_qspi_1bit_write_blocking(uint8_t *buf, size_t len) {
    for (size_t j = 0; j < len; j++) {
        // Split each byte into 4 nibbles (2 bits each)
        // IMPORTANT: Create nibbles in reverse order, then send in forward order
        uint8_t nibbles[4];
        for (int i = 0; i < 4; i++) {
            uint8_t bit1 = (buf[j] & (1 << (2 * i))) ? 1 : 0;
            uint8_t bit2 = (buf[j] & (1 << (2 * i + 1))) ? 1 : 0;
            nibbles[3 - i] = bit1 | (bit2 << 4);  // Store in reverse order!
        }

        // Send nibbles in forward order
        for (int i = 0; i < 4; i++) {
            while (pio_sm_is_tx_fifo_full(pio, sm))
                tight_loop_contents();
            *(volatile uint8_t *)&(pio->txf[sm]) = nibbles[i];
        }
    }

    // CRITICAL: Drain the state machine to ensure all data is shifted out
    pio_sm_drain_tx_fifo_inline(pio, sm);
}

// 4-bit mode: Send bytes directly for fast QSPI transfer
// Used for pixel data
void pio_qspi_4bit_write_blocking(uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        while (pio_sm_is_tx_fifo_full(pio, sm))
            tight_loop_contents();
        *(volatile uint8_t *)&(pio->txf[sm]) = buf[i];
    }

    // CRITICAL: Drain the state machine to ensure all data is shifted out
    pio_sm_drain_tx_fifo_inline(pio, sm);
}
