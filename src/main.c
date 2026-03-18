#include "pico/stdlib.h"

int main(void) {
    stdio_init_all();

    while (1) {
        tight_loop_contents();
    }

    return 0;
}
