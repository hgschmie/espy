/*
 * Manage all things display.
 */

#include <espy.h>
#include <EspyDisplay.h>


EspyDisplay::EspyDisplay(EspyHardware &_hardware)
        : hardware(_hardware) {}

EspyDisplayBuffer *current = nullptr;

void EspyDisplay::refresh() {
    if (current != nullptr) {
        hardware.leds(current->led);
    }
}

void EspyDisplay::display(EspyDisplayBuffer *buf) {
    current = buf;
}
