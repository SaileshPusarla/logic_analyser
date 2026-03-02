#include "stream.h"

void stream_init(void) { /* USB or SUMP init */ }
bool stream_data(const uint32_t *buf, size_t n_words) { return true; }
void stream_start(void) { /* start streaming */ }
void stream_stop(void) { /* stop streaming */ }
bool stream_ready(void) { return true; }
