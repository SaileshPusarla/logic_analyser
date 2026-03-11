#ifndef STREAM_H
#define STREAM_H

#include <stdint.h>
#include <stdbool.h>


// Initialize the stream subsystem
void stream_init(void);

// Non-blocking task to be called continuously in your main while(1) loop
void stream_task(void);


#endif // STREAM_H
