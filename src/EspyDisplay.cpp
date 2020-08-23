/*
 * Manage all things display.
 */

#include <espy.h>

const char *LED_state_names[] = {".", "O", "I", "S", "F"};

EspyDisplayBuffer::EspyDisplayBuffer(const char *name)
        : _name(name) {
    clear();
};

const char *EspyDisplayBuffer::name() {
    return _name;
}

void EspyDisplayBuffer::clear() {
    for (auto &i : text) {
        memset(i, ' ', DISPLAY_COLS);
        i[DISPLAY_COLS] = '\0';
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

void EspyDisplay::refresh() {
    fast.blink();
    slow.blink();

    if (current != nullptr) {
        if (current->render_and_reset()) {
            const char *buf[DISPLAY_ROWS];
            for (int i = 0; i < DISPLAY_ROWS; i++) {
                buf[i] = (char *) current->text[i];
            }
            hardware.text(buf);
        }

        // LEDs must not be controlled by the "render and reset" flag, as
        // they need to be contiuously rendered (otherwise they won't blink)
        hardware.leds(compute_led_state());
    }
}

uint8_t EspyDisplay::compute_led_state() const {

    uint8_t led = 0x1fu;
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
    return led;
}

void EspyDisplay::display(EspyDisplayBuffer *buf) {
    current = buf;
    if (current != nullptr) {
        current->request_render();
    }
}
