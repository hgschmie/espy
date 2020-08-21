/* -*- mode: C++; -*-
 *
 * Main include file for all espy code.
 */

#ifndef _ESPY_H_
#define _ESPY_H_

#include <TaskSchedulerDeclarations.h>

#include <EspyHardware.h>
#include <EspyBlinker.h>
#include <EspyDisplay.h>
#include <EspyKeys.h>
#include <menu.h>
#include <CustomWifiManager.h>

// run selfcheck on the system
// only enable active tasks if everything is ok
boolean self_check(EspyDisplayBuffer *);

// display refresh task
void display_task();

// keyboard scan task
void keyboard_task();

// wifi setup
void wifi_setup_activate(uint8_t param);

// dns
void dns_enable();

void dns_disable();

void dns_setup(Scheduler &scheduler);

// config portal
void wifi_setup(Scheduler &scheduler);

void wifi_config_mode();

void wifi_connect_mode();

extern EspyDisplayBuffer menu_buffer;

extern EspyDisplay *display;
extern LCDMenuLib2 LCDML;

extern CustomWiFiManager *wifiManager;

#endif // _ESPY_H_
