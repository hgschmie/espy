/* -*- mode: C++; -*-
 *
 * Menu code
 */

#include <espy.h>

EspyDisplayBuffer menu_buffer;

// housekeeping functions
void lcdml_menu_display();
void lcdml_menu_clear();

// internal stuff
void lcdml_menu_back(uint8_t);
void lcdml_screensaver(uint8_t);

LCDMenuLib2_menu LCDML_0(255, 0, 0, nullptr, nullptr); // root menu element (do not change)
// no menuControl callback, as the buttons are actively managed by the EspyKey controller.
LCDMenuLib2 LCDML(LCDML_0, DISPLAY_ROWS, DISPLAY_COLS, lcdml_menu_display, lcdml_menu_clear, nullptr);
// LCDML_add(id, prev_layer, new_num, lang_char_array, callback_function)
LCDML_add         (0, LCDML_0, 1, "Status", nullptr);
LCDML_add         (1, LCDML_0_1, 1, "* WiFi", nullptr);                    // NULL = no menu function
LCDML_add         (2, LCDML_0_1, 2, "* MQTT", nullptr);                    // NULL = no menu function
LCDML_add         (3, LCDML_0_1, 3, "< Back", lcdml_menu_back);
LCDML_add         (4, LCDML_0, 2, "Settings", nullptr);
// menu element count - last element id
// this value must be the same as the last menu element
#define _LCDML_DISP_cnt    4

// create menu
LCDML_createMenu(_LCDML_DISP_cnt);

void func_enter() {
    LCDML.BT_enter();
}

void func_up() {
    LCDML.BT_up();
}

void func_down() {
    LCDML.BT_down();
}

void func_quit() {
    LCDML.BT_quit();
}

void func_left() {
    LCDML.BT_left();
}

void func_right() {
    LCDML.BT_right();
}

//
// initialize the menu structure.
//
EspyDisplayBuffer *menu_setup(EspyKeys *keys) {
    // --- TEST ---
    // LCDMenuLib Setup
    LCDML_setup(_LCDML_DISP_cnt);

    // Some settings which can be used

    // Enable Menu Rollover
    LCDML.MENU_enRollover();

    // Enable Screensaver (screensaver menu function, time to activate in ms)
    LCDML.SCREEN_enable(lcdml_screensaver, 60000);

    keys->keys[0].on_release = func_up;
    keys->keys[0].on_long_press = func_left;
    keys->keys[1].on_release = func_down;
    keys->keys[1].on_long_press = func_right;
    keys->keys[2].on_release = func_enter;
    keys->keys[2].on_long_press = func_quit;

    // --- TEST ---

    return &menu_buffer;
}

//
// called by the scheduler to drive the menu code
//
void menu_task() {
    LCDML.loop();
}

void lcdml_menu_clear() {
    menu_buffer.clear();
}

void lcdml_menu_display() {
    if (LCDML.DISP_checkMenuUpdate()) {
        // clear menu
        // ***************
        LCDML.DISP_clear();

        // declaration of some variables
        // ***************
        // content variable
        char content_text[DISPLAY_COLS];  // save the content text of every menu element
        // menu element object
        LCDMenuLib2_menu *tmp;
        // some limit values
        uint8_t i = LCDML.MENU_getScroll();
        uint8_t maxi = DISPLAY_ROWS + i;
        uint8_t n = 0;

        // check if this element has children
        if ((tmp = LCDML.MENU_getDisplayedObj()) != nullptr) {
            // loop to display lines
            do {
                // check if a menu element has a condition and if the condition be true
                if (tmp->checkCondition()) {
                    // check the type off a menu element
                    if (tmp->checkType_menu() == true) {
                        // display normal content
                        LCDML_getContent(content_text, tmp->getID())
                        menu_buffer.lcd_print_P(n, PSTR(" %s"), content_text);
                    } else {
                        if (tmp->checkType_dynParam()) {
                            tmp->callback(n);
                        }
                    }
                    // increment some values
                    i++;
                    n++;
                }
                // try to go to the next sibling and check the number of displayed rows
            } while (((tmp = tmp->getSibling(1)) != nullptr) && (i < maxi));
        }
    }

    if (LCDML.DISP_checkMenuCursorUpdate()) {
        // init vars
        uint8_t n_max = (LCDML.MENU_getChilds() >= DISPLAY_ROWS) ? DISPLAY_ROWS : LCDML.MENU_getChilds();

        // display rows
        for (uint8_t n = 0; n < n_max; n++) {
            if (n == LCDML.MENU_getCursorPos()) {
                menu_buffer.text[n][0] = '>';
            }
        }
    }
}

void lcdml_screensaver(uint8_t param) {
    if (LCDML.FUNC_setup()) {
        // remmove compiler warnings when the param variable is not used:
        LCDML_UNUSED(param);

        // update LCD content
        menu_buffer.lcd_print_P(0, PSTR("press any key"));
        LCDML.FUNC_setLoopInterval(100);
    }

    if (LCDML.FUNC_loop()) {
        if (LCDML.BT_checkAny()) {
            LCDML.FUNC_goBackToMenu();
        }
    }

    if (LCDML.FUNC_close()) {
        // The screensaver goes to the root menu
        LCDML.MENU_goRoot();
    }
}


void lcdml_menu_back(uint8_t param) {
    if (LCDML.FUNC_setup()) {
        // remmove compiler warnings when the param variable is not used:
        LCDML_UNUSED(param);
        // end function and go an layer back
        LCDML.FUNC_goBackToMenu(1);      // leave this function and go a layer back
    }
}
