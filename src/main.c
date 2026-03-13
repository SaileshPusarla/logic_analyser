#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "capture.h"  // Ensure path matches your project structure

#define TEST_SIGNAL_PIN 2

void core1_entry() {
    printf("Core 1: Started and waiting for data...\n");
    while (1) {
        if (capture_is_done()) {
            capture_buffer_t cap = capture_get_buffer();
            printf("\n--- DATA RECEIVED (%d bytes) ---\n", cap.length);
            
            // Print only the first 50 samples to be safe
            for (size_t i = 0; i < 50; i++) {
                uint8_t val = cap.data[i];
                for (int b = 7; b >= 0; b--) {
                    printf("%c", (val & (1 << b)) ? '|' : '.');
                }
                printf(" [0x%02X]\n", val);
            }
            printf("--- END PREVIEW ---\n");
            
            // Crucial: Clear the "done" state if you have a mechanism, 
            // or just rely on capture_start() resetting it.
            sleep_ms(2000); 
        }
        tight_loop_contents();
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000); // Give you time to open picocom
    printf("System Initializing...\n");

    capture_init(pio0, 0, 5000000.0f);
    multicore_launch_core1(core1_entry);

    gpio_init(TEST_SIGNAL_PIN);
    gpio_set_dir(TEST_SIGNAL_PIN, GPIO_OUT);

    while (1) {
        printf("Main: Starting Capture (Waiting for Falling Edge on GPIO 2)...\n");
        capture_start();
        
        // Force a toggle to guarantee a falling edge
        gpio_put(TEST_SIGNAL_PIN, 1);
        sleep_ms(10);
        gpio_put(TEST_SIGNAL_PIN, 0); // This should trigger the PIO!
        
        // Generate a 1kHz clock for a bit
        for(int i = 0; i < 1000; i++) {
            gpio_put(TEST_SIGNAL_PIN, 1);
            sleep_us(500);
            gpio_put(TEST_SIGNAL_PIN, 0);
            sleep_us(500);
        }

        printf("Main: Signals generated. Waiting for DMA to fill 384KB...\n");
        capture_wait();
        printf("Main: Capture Complete!\n");
        
        sleep_ms(3000); 
    }
}

