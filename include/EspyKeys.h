/* -*- mode: C++; -*-
 *
 * Manage the keys on the keyboard
 */

#include <Arduino.h>
#include <TaskSchedulerDeclarations.h>

#ifndef _ESPY_ESPYKEYS_H_
#define _ESPY_ESPYKEYS_H_

#include <espy.h>

// run the timer task every 10 ms
#define KEY_TIMER_MS 10

// debounce time: 100 ms
#define DEBOUNCE_TIME_MS 100

// long press time: 2 seconds
#define LONG_PRESS_TIME_MS 2000

typedef void (*key_func)(void);

struct key_control {
    bool pressed = false;
    bool long_pressed = false;
    int press_count = 0;
    int release_count = 0;
    key_func on_press = nullptr;
    key_func on_long_press = nullptr;
    key_func on_release = nullptr;
};


class EspyKeys {
public:
    explicit EspyKeys(EspyHardware &_hardware);

    key_control keys[3];

    void scan();

    uint8_t state();

private:
    EspyHardware &hardware;         // Reference to the detected hardware
};


#endif //ESPY_ESPYKEYS_H
