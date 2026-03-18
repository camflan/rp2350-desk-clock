#include "pio_qspi.h"
#include "pio_qspi.pio.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

static PIO qspi_pio;
static uint qspi_sm;

// Wait until every byte has been shifted out on the wire.
// Checking FIFO empty alone is not enough — the OSR may still
// be mid-shift, so we also wait for the TX-stall flag.
static void drain_tx(void) {
    while (!pio_sm_is_tx_fifo_empty(qspi_pio, qspi_sm))
        tight_loop_contents();

    uint32_t stall = 1u << (PIO_FDEBUG_TXSTALL_LSB + qspi_sm);
    qspi_pio->fdebug = stall;          // clear
    while (!(qspi_pio->fdebug & stall))
        tight_loop_contents();          // wait for re-assert
}

void pio_qspi_init(uint clk_pin, uint d0_pin, uint baudrate) {
    qspi_pio = pio0;
    qspi_sm  = 0;

    // PIO runs 2 instructions per bit-clock → divide by 2×baudrate
    float div = (float)clock_get_hz(clk_sys) / (baudrate * 2.0f);
    if (div < 1.0f)
        div = 1.0f;

    uint offset = pio_add_program(qspi_pio, &pio_qspi_program);
    pio_qspi_program_init(qspi_pio, qspi_sm, offset, clk_pin, d0_pin, div);
}

/*
 * 1-bit (SPI-compatible) write.
 *
 * The PIO program shifts 4 pins per clock, but in 1-bit mode only
 * D0 carries data (D1–D3 stay low). We expand each input byte into
 * 4 FIFO bytes so we get 8 clocks total (one per bit, MSB first).
 *
 * PIO byte-write replicates the byte across the 32-bit FIFO word,
 * then left-shifts it out in 4-bit nibbles:
 *
 *   FIFO byte  →  high nibble shifted first, low nibble second
 *   D0 = bit 0 of each nibble  →  bit 4 for high, bit 0 for low
 *
 * So to send bit_first then bit_second on D0:
 *   fifo_byte = (bit_first << 4) | bit_second
 */
void pio_qspi_1bit_write_blocking(const uint8_t *buf, size_t len) {
    for (size_t j = 0; j < len; j++) {
        uint8_t byte = buf[j];

        // Pack pairs of data bits (MSB first) into FIFO bytes.
        //   nibbles[0]: bit7 (high nib) then bit6 (low nib)
        //   nibbles[1]: bit5 then bit4
        //   nibbles[2]: bit3 then bit2
        //   nibbles[3]: bit1 then bit0
        uint8_t nibbles[4];
        for (int i = 0; i < 4; i++) {
            uint8_t first  = (byte >> (7 - 2 * i))     & 1;
            uint8_t second = (byte >> (7 - 2 * i - 1)) & 1;
            nibbles[i] = (first << 4) | second;
        }

        for (int i = 0; i < 4; i++) {
            while (pio_sm_is_tx_fifo_full(qspi_pio, qspi_sm))
                tight_loop_contents();
            *(volatile uint8_t *)&qspi_pio->txf[qspi_sm] = nibbles[i];
        }
    }

    drain_tx();
}

// 4-bit (native QSPI) write — each byte maps directly to 2 nibble
// shifts on D0–D3, so we just push bytes straight into the FIFO.
void pio_qspi_4bit_write_blocking(const uint8_t *buf, size_t len) {
    for (size_t i = 0; i < len; i++) {
        while (pio_sm_is_tx_fifo_full(qspi_pio, qspi_sm))
            tight_loop_contents();
        *(volatile uint8_t *)&qspi_pio->txf[qspi_sm] = buf[i];
    }

    drain_tx();
}
