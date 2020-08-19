/* -*- mode: C++; -*-
 *
 * Main include file for all espy code.
 */

#ifndef _ESPY_H_
#define _ESPY_H_

#include <EspyHardware.h>
#include <EspyBlinker.h>
#include <EspyDisplay.h>
#include <EspyKeys.h>
#include <menu.h>

// run selfcheck on the system
// only enable active tasks if everything is ok
boolean self_check(EspyDisplayBuffer *);

// display refresh task
void display_task();

// keyboard scan task
void keyboard_task();

#endif // _ESPY_H_
