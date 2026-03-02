// ui.c
#include "ui.h"

void ui_init(void) { /* buttons, LEDs */ }
void ui_poll(void) { /* update states */ }
bool ui_trigger_pressed(void) { return false; }
trigger_type_t ui_get_trigger_type(void) { return TRIGGER_RISING_EDGE; }
void ui_display_message(const char *msg) { /* optional */ }
