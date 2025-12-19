// src/main.c
#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    stdio_init_all();

    printf("RP2350 AMOLED Clock starting...\n");

    while (1) {
        printf("Hello from RP2350!\n");
        sleep_ms(1000);
    }

    return 0;
}
