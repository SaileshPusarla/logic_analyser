#include "stream.h"
#include "capture.h"

#include <stdio.h>

#include "pico/stdio.h"
#include "pico/error.h"

void stream_task(void)
{
    int cmd = getchar_timeout_us(0);

    if (cmd == PICO_ERROR_TIMEOUT)
        return;

    switch (cmd)
    {
        case 0x00:  // reset
            break;

        case 0x02:  // ID request
            putchar('1');
            putchar('A');
            putchar('L');
            putchar('S');
            fflush(stdout);
            break;

        case 0x01:  // ARM
        {
            capture_start();
            capture_wait();

            capture_buffer_t cap = capture_get_buffer();

            fwrite(cap.data, 1, cap.length, stdout);
            fflush(stdout);

            break;
        }

        default:
            break;
    }
}
