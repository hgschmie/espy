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
#include <espy.h>

#include <TaskScheduler.h>
#include "../.pio/libdeps/esp01_1m/TaskScheduler/src/TaskSchedulerDeclarations.h"

EspyHardware *hardware;
EspyDisplay *display;

Scheduler scheduler;

boolean self_check();

void display_task();
void led_task();
void lcd_task();

Task displayTask(20, TASK_FOREVER, &display_task);

Task lcdTask(1000, TASK_FOREVER, &lcd_task);

EspyDisplayBuffer buf;

/*
 * Run all the setup code before the main loop hits.
 */
void setup() {
    Serial.begin(9600);
    Serial.println("Setup starting");

    // bring up display and led hardware
    hardware = new EspyHardware();
    display = new EspyDisplay(*hardware);

    display->display(&buf);

    scheduler.init();

    // start the background display refresh task
    scheduler.addTask(displayTask);
    scheduler.addTask(lcdTask);
//    scheduler.addTask(ledTask);

    displayTask.enable();

    // Enable all other tasks only if the hardware is ok.
    if (self_check()) {
        lcdTask.enable();
    }

    buf.leds[1] = led_state::ON;
    buf.leds[2] = led_state::SLOW;
    buf.leds[3] = led_state::FAST;
}

boolean self_check() {
    if (hardware->error == HW_NO_DISPLAY_FOUND) {
        buf.leds[0] = led_state::FAST;
    } else if (hardware->error == HW_NO_PCF_FOUND) {
        buf.leds[0] = led_state::SLOW; // actually pointless. :-)
    }

    return hardware->error == HW_NO_ERROR; // true if all is fine
}


void display_task() {
    display->refresh();
    Serial.print(".");
}

uint8_t count = 0;
boolean up = true;

void lcd_task() {
    buf.text[0] = String("This is a testy ");
    lcdTask.disable();
}

void loop() {
    for (;;) {
        scheduler.execute();
    }
}
