/*
 * Manage all things display.
 */

#include <espy.h>

EspyDisplayBuffer::EspyDisplayBuffer(const char *name)
        : _name(name) {
    clear();
};

const char *EspyDisplayBuffer::name() {
    return _name;
}

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
        : current(nullptr), hardware(_hardware),
          fast(EspyBlinker(BLINK_FAST)), slow(EspyBlinker(BLINK_SLOW)) {
}

#ifdef _ESPY_DEBUG_BUFFERS
char *_espy_debug_buf[2] = { new char(17), new char(17) };
#endif

void EspyDisplay::refresh() {
    uint8_t led = 0x00u;
    if (current != nullptr) {
        led = compute_led_state();

#ifdef _ESPY_DEBUG_BUFFERS
        snprintf(_espy_debug_buf[0], 16, "%s", current->name());
        snprintf(_espy_debug_buf[1], 16, "%02x", led);
        hardware.text((const char **)_espy_debug_buf);
#endif

        if (current->render_and_reset()) {
            const char *buf[DISPLAY_ROWS];
            for (int i = 0; i < DISPLAY_ROWS; i++) {
                buf[i] = (char *) current->text[i];
            }
            hardware.text(buf);
        }
    }
    hardware.leds(led);
}

uint8_t EspyDisplay::compute_led_state() {

    fast.blink();
    slow.blink();

    uint8_t led = 0x00u;
    if (current != nullptr) {
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
    return led;
}

void EspyDisplay::display(EspyDisplayBuffer *buf) {
    current = buf;
    if (current != nullptr) {
        current->request_render();
    }
}
