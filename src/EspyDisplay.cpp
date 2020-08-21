/*
 * Manage all things display.
 */

#include <espy.h>

EspyDisplayBuffer::EspyDisplayBuffer() {
    clear();
};

void EspyDisplayBuffer::clear() {
    for (int i = 0; i < DISPLAY_ROWS; i++) {
        memset(text[i], DISPLAY_COLS, ' ');
        text[i][DISPLAY_COLS] = '\0';
    }

    request_render();
}

void EspyDisplayBuffer::lcd_print(int row, const char *fmt ...) {
    va_list argp;
    va_start(argp, fmt);
    vsnprintf(text[row], DISPLAY_COLS, fmt, argp);
    va_end(argp);

    request_render();
}

void EspyDisplayBuffer::lcd_print_P(int row, const char *fmt ...) {
    va_list argp;
    va_start(argp, fmt);
    vsnprintf_P(text[row], DISPLAY_COLS, fmt, argp);
    va_end(argp);

    request_render();
}

void EspyDisplayBuffer::request_render() {
    render = true;
}

bool EspyDisplayBuffer::render_and_reset() {
    bool res = render;
    render = false;
    return res;
}

EspyDisplay::EspyDisplay(EspyHardware &_hardware)
        : current(nullptr), hardware(_hardware), led(0x00u),
          fast(EspyBlinker(BLINK_FAST)), slow(EspyBlinker(BLINK_SLOW)) {
}

void EspyDisplay::refresh() {
    if (current != nullptr) {
        compute_led_state();
        hardware.leds(led);

        if (current->render_and_reset()) {
            const char *buf[DISPLAY_ROWS];
            for (int i = 0; i < DISPLAY_ROWS; i++) {
                buf[i] = (char *) current->text[i];
            }
            hardware.text(buf);
        }
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
    if (current != nullptr) {
        current->request_render();
    }
}
