#include "pico/stdlib.h"
#include "display_driver.h"

int main(void) {
    stdio_init_all();

    display_init();

    /* Smoke test: cycle red → green → blue */
    display_test_fill(0xF800);  // Red
    sleep_ms(1000);
    display_test_fill(0x07E0);  // Green
    sleep_ms(1000);
    display_test_fill(0x001F);  // Blue
    sleep_ms(1000);

    while (1) {
        tight_loop_contents();
    }

    return 0;
}
