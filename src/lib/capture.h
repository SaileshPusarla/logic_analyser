#ifndef CAPTURE_H
#define CAPTURE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#define CAPTURE_BUFFER_SIZE (256 * 1024)
#define CAPTURE_CHANNELS 8
#define CAPTURE_SAMPLE_RATE 5000000

typedef enum {
    TRIGGER_NONE = 0,
    TRIGGER_RISING,
    TRIGGER_FALLING,
    TRIGGER_HIGH,
    TRIGGER_LOW
} trigger_mode_t;

typedef struct {
    trigger_mode_t mode;
    uint8_t channel;
} trigger_config_t;

typedef struct {
    uint8_t *data;
    size_t length;
    uint32_t sample_rate;
    uint8_t channels;
} capture_buffer_t;

bool capture_init(trigger_config_t trig);
bool capture_start(void);
bool capture_wait(void);
capture_buffer_t capture_get_buffer(void);

#endif
