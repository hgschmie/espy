/*
 * Manage all things display.
 */

#include <espy.h>
#include <EspyDisplay.h>


EspyDisplay::EspyDisplay(EspyHardware &_hardware)
        : current(nullptr), hardware(_hardware), led(0x00u),
        fast(EspyBlinker(BLINK_FAST)), slow(EspyBlinker(BLINK_SLOW)) {
}

void EspyDisplay::refresh() {
    if (current != nullptr) {
        compute_led_state();
        hardware.leds(led);
        hardware.text(current->text);
    }
}

void EspyDisplay::compute_led_state() {

    fast.blink();
    slow.blink();

    for (unsigned int i = 0; i < 5; i++) {

        led_state state = current->leds[i];
        if (state == SLOW) {
            state = slow.state ? led_state::ON : led_state::OFF;
        } else if (state == FAST) {
            state = fast.state ? led_state::ON : led_state::OFF;
        }

        if (state == ON) {
            led |= bit(i);
        } else if (state == OFF) {
            led &= ~bit(i);
        }
    }
}

void EspyDisplay::display(EspyDisplayBuffer *buf) {
    current = buf;
}
