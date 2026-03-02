#ifndef ANALYZE_H
#define ANALYZE_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

void analyze_init(uint8_t pin_count);

//process a block of captured samples by taking pointer to the buffer
void analyze_process_block(const uint32_t *buf, size_t n_words);

//calculate frequency for a given channel
float analyze_channel_frequency(uint8_t channel);

//calculate jitter for a given channel
float analyze_channel_jitter(uint8_t channel);

// Reset analysis
void analyze_reset(void);

#endif
