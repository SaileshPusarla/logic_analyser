#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"

#include "capture.h"

static void print_byte_binary(uint8_t v)
{
    for (int i = 7; i >= 0; i--)
    {
        printf("%d", (v >> i) & 1);
    }
}

int main()
{
    stdio_init_all();

    sleep_ms(2000);  // allow USB enumeration

    printf("\n===== LOGIC ANALYZER TEST =====\n");
    printf("Program started\n");


    trigger_config_t trig = {
        .mode = TRIGGER_NONE,
        .channel = 0
    };

    printf("Calling capture_init...\n");

    if (!capture_init(trig))
    {
        printf("capture_init FAILED\n");
        while (1);
    }

    printf("capture_init OK\n");

    while (1)
    {
        printf("\n--- NEW CAPTURE ---\n");

        printf("Starting capture...\n");

        capture_start();

        printf("Waiting for DMA...\n");

        capture_wait();

        printf("DMA complete\n");

        capture_buffer_t cap = capture_get_buffer();

        printf("Captured %u bytes\n", (unsigned)cap.length);

        printf("\nFirst 32 samples (8 channels):\n");

        for (int i = 0; i < 32; i++)
        {
            print_byte_binary(cap.data[i]);
            printf("\n");
        }

        printf("\n");

        sleep_ms(500);
    }
}
