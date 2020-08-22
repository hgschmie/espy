/*
 * All things related to the display logic (both leds and lcd)
 */

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

#ifndef _ESPY_ESPYDISPLAY_H_
#define _ESPY_ESPYDISPLAY_H_

#include <espy.h>

enum led_state {
    ON,
    OFF,
    IGNORE,
    SLOW,
    FAST
};

#define LED_TOGGLE(x, n) (x)->leds[n] = ((x)->leds[n] == led_state::ON) ? led_state::OFF : led_state::ON

#define REFRESH_RATE (1000 / 50)
#define BLINK_SLOW (560 / REFRESH_RATE) // 2 second blink
#define BLINK_FAST (100 / REFRESH_RATE)  // 0.5 second blink

class EspyDisplay;

class EspyDisplayBuffer {
    friend class EspyDisplay;

public:
    EspyDisplayBuffer(const char *name);

    char text[DISPLAY_ROWS][DISPLAY_COLS + 1]{};
    led_state leds[5] = {OFF, OFF, OFF, OFF, OFF};

    const char *name();

    void clear();

    void lcd_print(int row, const char *fmt ...);

    void lcd_print_P(int row, const char *fmt ...);

    void request_render();

private:
    const char *_name;

    bool render_and_reset();

    bool render = false;
};

/*
 * Contains all the display information.
 */
class EspyDisplay {
public:
    explicit EspyDisplay(EspyHardware &_hardware);

    void refresh();

    void display(EspyDisplayBuffer *buf);

    EspyDisplayBuffer *current;     // Current display buffer. Will be rendered at refresh
    uint8_t compute_led_state();

private:
    EspyHardware &hardware;         // Reference to the detected hardware
    EspyBlinker fast;
    EspyBlinker slow;

};


#endif //ESPY_ESPYDISPLAY_H
