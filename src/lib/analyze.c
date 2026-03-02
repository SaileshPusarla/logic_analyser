#include "analyze.h"

void analyze_init(uint8_t pin_count) { /* allocate internal buffers */ }
void analyze_process_block(const uint32_t *buf, size_t n_words) { /* math */ }
float analyze_channel_frequency(uint8_t channel) { return 0.0f; }
float analyze_channel_jitter(uint8_t channel) { return 0.0f; }
void analyze_reset(void) { /* clear buffers */ }
