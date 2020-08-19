/* -*- mode: C++; -*-
 *
 * Menu code
 */

#ifndef _MENU_H_
#define _MENU_H_

#include <LCDMenuLib2.h>

// Call from setup function to initialize menu
EspyDisplayBuffer *menu_setup(EspyKeys *);

// call from scheduler task to update menu
void menu_task();



#endif
