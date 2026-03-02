#include "capture.h"
#include "analyze.h"
#include "stream.h"
#include "ui.h"
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include <stdlib.h>
capture_config_t cfg = {
    .pin_base = 16,
    .pin_count = 8,
    .sample_freq_mhz = 5.0f,
    //size of the buffer
    .buffer_size_words = 256*1024,  
    //placeholder, allocated dynamically later on 
    .buffer = NULL
};

// Core 1: Analysis + math
void core1_entry() {
    analyze_init(cfg.pin_count);
    while (true) {
        if (capture_done()) {
            uint32_t *buf = capture_get_buffer();
            analyze_process_block(buf, cfg.buffer_size_words);
            // Ready for streaming
        }
    }
}

int main() {
    stdio_init_all();

    // Allocate buffer
    cfg.buffer = malloc(cfg.buffer_size_words * sizeof(uint32_t));

    ui_init();
    stream_init();
    capture_init(&cfg);

    // Launch core 1 for analysis
    multicore_launch_core1(core1_entry);

    while (true) {
        ui_poll();

        if (ui_trigger_pressed()) {
            capture_arm(cfg.pin_base, true);
        }

        if (capture_done()) {
            uint32_t *buf = capture_get_buffer();
            if (stream_ready()) {
                stream_data(buf, cfg.buffer_size_words);
            }
            capture_reset();
        }
    }
}
