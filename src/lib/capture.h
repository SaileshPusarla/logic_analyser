#ifndef _CAPTURE_H_
#define _CAPTURE_H_

#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "hardware/dma.h"

#define CAPTURE_PIN_BASE 2
#define CAPTURE_PIN_COUNT 8
#define CAPTURE_BUFFER_SIZE_BYTES (384 * 1024)

// This struct fixes the errors in stream.c
typedef struct {
    uint8_t* data;
    size_t length;
} capture_buffer_t;

// API Functions
void capture_init(PIO pio, uint sm, float freq_hz);
void capture_start(void);
void capture_wait(void); // Blocking wait for stream.c
bool capture_is_done(void);
capture_buffer_t capture_get_buffer(void);

#endif
