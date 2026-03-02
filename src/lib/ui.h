#ifndef UI_H
#define UI_H

#include <stdbool.h>

typedef enum {
    TRIGGER_RISING_EDGE,
    TRIGGER_FALLING_EDGE,
} trigger_type_t;

void ui_init(void);
void ui_poll(void);
bool ui_trigger_pressed(void);
trigger_type_t ui_get_trigger_type(void);
void ui_display_message(const char *msg);

#endif
