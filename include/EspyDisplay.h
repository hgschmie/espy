/*
 * All things related to the display logic (both leds and lcd)
 */
#ifndef ESPY_ESPYDISPLAY_H
#define ESPY_ESPYDISPLAY_H

#include <espy.h>

enum led_state {
    ON,
    OFF,
    IGNORE,
    SLOW,
    FAST
};

#define REFRESH_RATE (1000 / 50)
#define BLINK_SLOW (560 / REFRESH_RATE) // 2 second blink
#define BLINK_FAST (100 / REFRESH_RATE)  // 0.5 second blink

struct EspyDisplayBuffer {
    String text[2] = {"", ""};
    led_state leds[5] = {OFF, OFF, OFF, OFF, OFF};
};

/*
 * Contains all the display information.
 */
class EspyDisplay {
public:
    explicit EspyDisplay(EspyHardware &_hardware);

    void refresh();

    void display(EspyDisplayBuffer *buf);

private:
    EspyHardware &hardware;         // Reference to the detected hardware
    EspyDisplayBuffer *current;     // Current display buffer. Will be rendered at refresh
    uint8_t led;                    // Current LED state.
    EspyBlinker fast;
    EspyBlinker slow;

    void compute_led_state();
};


#endif //ESPY_ESPYDISPLAY_H
