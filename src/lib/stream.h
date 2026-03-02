#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
//initializes the stream
void stream_init(void);
bool stream_data(const uint32_t *buf, size_t n_words);
void stream_start(void);
void stream_stop(void);
bool stream_ready(void);

#endif
