#include <stdio.h>
#include "pico/stdlib.h"

int main() {
    // 1. Initialize all standard IO (enables USB/UART)
    stdio_init_all();

    // 2. Initialize the onboard LED
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while (true) {
        // Blink the LED
        gpio_put(LED_PIN, 1);
        printf("Hello from Pico! LED is ON\n");
        sleep_ms(500);

        gpio_put(LED_PIN, 0);
        printf("Hello from Pico! LED is OFF\n");
        sleep_ms(500);
    }
}
