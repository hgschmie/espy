/*
 * All things related to the display logic (both leds and lcd)
 */

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

#ifndef _ESPY_ESPYDISPLAY_H_
#define _ESPY_ESPYDISPLAY_H_

#include <espy.h>

enum led_state {
    OFF = 0,
    ON = 1,
    IGNORE = 2,
    SLOW = 3,
    FAST = 4
};

#define LED_TOGGLE(x, n) (x)->leds[n] = ((x)->leds[n] == led_state::ON) ? led_state::OFF : led_state::ON

extern const char *LED_state_names[];

#define LED_STATE(x, n) LED_state_names[(int)((x)->leds[n])]

#define REFRESH_RATE (1000 / 50)
#define BLINK_SLOW (560 / REFRESH_RATE) // 2 second blink
#define BLINK_FAST (100 / REFRESH_RATE)  // 0.5 second blink

class EspyDisplay;

class EspyDisplayBuffer {
    friend class EspyDisplay;

public:

    explicit EspyDisplayBuffer(const char *name);

    led_state leds[5] = {OFF, OFF, OFF, OFF, OFF};
    char text[DISPLAY_ROWS][DISPLAY_COLS + 1]{};

    const char *name();

    void clear();

    void lcd_print(int row, const char *fmt ...);

    void lcd_print_P(int row, const char *fmt ...);

    void request_render();

private:
    const char *_name;
    bool render = false;

    bool render_and_reset();
};

/*
 * Contains all the display information.
 */
class EspyDisplay {
public:
    EspyDisplayBuffer *current;     // Current display buffer. Will be rendered at refresh

    explicit EspyDisplay(EspyHardware &_hardware);

    void refresh();

    void display(EspyDisplayBuffer *buf);

private:
    EspyHardware &hardware;         // Reference to the detected hardware
    EspyBlinker fast;
    EspyBlinker slow;

    uint8_t compute_led_state() const;
};


#endif //ESPY_ESPYDISPLAY_H
