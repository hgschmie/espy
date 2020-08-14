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

void display_task();

void led_task();

EspyDisplayBuffer buf;
Task displayTask(20, TASK_FOREVER, &display_task);
Task ledTask(100, TASK_FOREVER, &led_task);

/*
 * Run all the setup code before the main loop hits.
 */
void setup() {
    Serial.begin(9600);
    Serial.println("Setup starting");

    // bring up display and led hardware
    hardware = new EspyHardware();
    display = new EspyDisplay(*hardware);


    scheduler.init();

    // start the background display refresh task
    scheduler.addTask(displayTask);
    scheduler.addTask(ledTask);

    buf.led = 0x00u;
    display->display(&buf);

    Serial.println("Tasks enabling...");

    displayTask.enable();
    ledTask.enable();

    Serial.println("Setup ok");
}

void display_task() {
    display->refresh();
}

uint8_t count = 0;
boolean up = true;

void led_task() {
    if (up) {
        if (count == 4) {
            up = false;
        }
        buf.led |= (1u << count++);
    } else {
        if (count == 0) {
            up = true;
        }
        buf.led &= ~ (1u << count--);
    }
}

void loop() {
    for (;;) {
        scheduler.execute();
    }
}
