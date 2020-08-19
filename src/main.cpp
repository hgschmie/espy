/*
 * Main entry point for the espy program.
 *
 * Setup creates all the necessary stuff to get Wifi and the hardware going.
 *
 * Anything before the main loop goes into setup.
 *
 * loop is the main loop.
 */


#include <Arduino.h>
#include <TaskScheduler.h>

#include <espy.h>

EspyHardware *hardware;
EspyDisplay *display;
EspyKeys *keys;

Scheduler scheduler;

// scheduler tasks
Task displayTask(20, TASK_FOREVER, &display_task);
Task keyboardTask(KEY_TIMER_MS, TASK_FOREVER, &keyboard_task);
Task menuTask(100, TASK_FOREVER, &menu_task);

EspyDisplayBuffer buf;

/*
 * Run all the setup code before the main loop hits.
 */
void setup() {
#ifdef _ESPY_DEBUG
    Serial.begin(9600);
    Serial.println("Setup starting");
#endif

    // bring up display and led hardware
    hardware = new EspyHardware();
    display = new EspyDisplay(*hardware);
    keys = new EspyKeys(*hardware);

    // point display at the current buf
    display->display(&buf);

    // bring up the scheduler
    scheduler.init();

    // start the background display refresh task
    scheduler.addTask(displayTask);
    displayTask.enable();

    scheduler.addTask(keyboardTask);
    keyboardTask.enable();

    scheduler.addTask(menuTask);

    // bring up the system tasks

    // Enable all other tasks only if the hardware is ok.
    if (self_check(&buf)) {
        // enable other tasks here

        EspyDisplayBuffer *menu_buffer = menu_setup(keys);
        menuTask.enable();

        menu_buffer->lcd_print_P(0, PSTR("Hello"));
        menu_buffer->lcd_print_P(1, PSTR("World"));

        menu_buffer->leds[0] = led_state::FAST;
        display->display(menu_buffer);
    }
}

boolean self_check(EspyDisplayBuffer *sc_buf) {
    if (hardware->error == HW_NO_DISPLAY_FOUND) {
        sc_buf->leds[0] = led_state::FAST;
    } else if (hardware->error == HW_NO_PCF_FOUND) {
        sc_buf->lcd_print_P(0, PSTR("NO PCF CHIP FOUND!"));
    }

    return hardware->error == HW_NO_ERROR; // true if all is fine
}


void display_task() {
    display->refresh();
}

void keyboard_task() {
    keys->scan();
}

void loop() {
    scheduler.execute();
}


