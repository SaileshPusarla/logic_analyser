#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>
#include <stdbool.h>


// --- General Initialization ---
void capture_init(void);

// --- Digital Mode (Snapshot) ---
void capture_digital_config(uint32_t sample_rate_hz, uint32_t sample_count);
void capture_digital_start(void);
bool capture_digital_done(void);
uint8_t* capture_digital_get_buffer(void);
uint32_t capture_digital_get_sample_count(void);

// --- Analog Mode (Continuous Triple Buffer) ---
void capture_analog_config(uint32_t sample_rate_hz);
void capture_analog_start(void);

// Called in the main loop to check if a buffer is ready for math/USB
uint16_t* capture_analog_get_ready_buffer(void);

// Called in the main loop to give the buffer back to the DMA system
void capture_analog_release_buffer(void);

#ifdef __cplusplus
}
#endif

#endif H
