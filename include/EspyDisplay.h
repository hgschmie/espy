/*
 * All things related to the display logic (both leds and lcd)
 */
#ifndef ESPY_ESPYDISPLAY_H
#define ESPY_ESPYDISPLAY_H

#include <Arduino.h>

struct EspyDisplayBuffer {
    String text[2] = {"", ""};
    uint8_t led = 0x00;
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
    EspyHardware &hardware;
};


#endif //ESPY_ESPYDISPLAY_H
