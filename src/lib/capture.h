#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct {
    uint8_t pin_base;
    uint8_t pin_count;
    float sample_freq_mhz;
    uint32_t *buffer;
    size_t buffer_size_words;
} capture_config_t;

// Initialize PIO, DMA, buffers
void capture_init(const capture_config_t *config);

// Arm capture with trigger
void capture_arm(uint8_t trigger_pin, bool trigger_level);

// Check if capture is done
bool capture_done(void);

// Get pointer to capture buffer
uint32_t* capture_get_buffer(void);

// Reset capture
void capture_reset(void);

#endif
