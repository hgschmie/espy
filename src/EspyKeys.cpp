/*
 * Manage all things keys.
 */

#include <espy.h>

EspyKeys::EspyKeys(EspyHardware &_hardware)
        : hardware(_hardware) {
}

extern uint8_t debug_keys;

void EspyKeys::scan() {
    uint8_t key_state = hardware.keys();

    for (unsigned int i = 0; i < 3; i++) {
        uint8_t bit = 4u >> i; // keys are in the wrong order, so 1 2 4 --> 4 2 1
        key_control *control = &keys[i];
        if (key_state & bit) {
            // key pressed
            control->release_count = 0;
            if (++control->press_count > (DEBOUNCE_TIME_MS / KEY_TIMER_MS)) {
                if (!control->pressed) {
                    control->pressed = true;
                    if (control->on_press != nullptr) {
                        control->on_press();
                    }
                }

                if (control->press_count > (LONG_PRESS_TIME_MS / KEY_TIMER_MS)) {
                    if (!control->long_pressed) {
                        control->long_pressed = true;
                        if (control->on_long_press != nullptr) {
                            control->on_long_press();
                        }
                    }
                }
            }
        } else {
            // key released
            control->press_count = 0;
            if (control->pressed && (++control->release_count > (DEBOUNCE_TIME_MS / KEY_TIMER_MS))) {
                bool long_pressed = control->long_pressed;

                control->pressed = false;
                control->long_pressed = false;

                // only call "on release" if not pressed long.
                if (!long_pressed && control->on_release != nullptr) {
                    control->on_release();
                }
            }
        }
    }
}

uint8_t EspyKeys::state() {
    uint8_t result = 0x00;
    for (unsigned int i = 0; i < 3; i++) {
        if (keys[i].pressed) {
            result |= 1 << i;
        }
        if (keys[i].long_pressed) {
            result |= 8 << i;
        }
    }
    return result;
}
