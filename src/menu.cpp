/* -*- mode: C++; -*-
 *
 * Menu code
 */

#include <ESP8266WiFi.h>
#include <espy.h>

EspyDisplayBuffer menu_buffer;

// housekeeping functions
void lcdml_menu_display();

void lcdml_menu_clear();

// internal stuff
void lcdml_menu_back(uint8_t);

void lcdml_screensaver(uint8_t);

bool always_false() {
    return false;
}

String param_name[7] =
        {
                "Wifi SSID",
                "Retry Count",
                "IP",
                "Gateway",
                "DNS",
                "Hostname",
                "LEDs"
        };


void settings(uint8_t);


LCDMenuLib2_menu LCDML_0(255, 0, 0, nullptr, nullptr); // root menu element (do not change)
// no menuControl callback, as the buttons are actively managed by the EspyKey controller.
LCDMenuLib2 LCDML(LCDML_0, DISPLAY_ROWS, DISPLAY_COLS, lcdml_menu_display, lcdml_menu_clear, nullptr);
// LCDML_add(id, prev_layer, new_num, lang_char_array, callback_function)
LCDML_add         (0, LCDML_0, 1, "Status", nullptr);
LCDML_add         (1, LCDML_0_1, 1, "WiFi", nullptr);
LCDML_addAdvanced (2, LCDML_0_1_1, 1, NULL, "Wifi SSID", settings, 0, _LCDML_TYPE_default);
LCDML_addAdvanced (3, LCDML_0_1_1, 2, NULL, "Retry Count", settings, 1, _LCDML_TYPE_default);
LCDML_addAdvanced (4, LCDML_0_1_1, 3, NULL, "IP", settings, 2, _LCDML_TYPE_default);
LCDML_addAdvanced (5, LCDML_0_1_1, 4, NULL, "Gateway", settings, 3, _LCDML_TYPE_default);
LCDML_addAdvanced (6, LCDML_0_1_1, 5, NULL, "DNS", settings, 4, _LCDML_TYPE_default);
LCDML_addAdvanced (7, LCDML_0_1_1, 6, NULL, "Hostname", settings, 5, _LCDML_TYPE_default);
LCDML_addAdvanced (8, LCDML_0_1_1, 7, NULL, "LEDs", settings, 6, _LCDML_TYPE_default);
LCDML_add         (9, LCDML_0_1_1, 8, "< Back", lcdml_menu_back);
LCDML_add         (10, LCDML_0_1, 2, "MQTT", nullptr);
LCDML_add         (11, LCDML_0_1, 3, "< Back", lcdml_menu_back);
LCDML_add         (12, LCDML_0, 2, "Settings", nullptr);
// LCDML_add         (6, LCDML_0_2, 1, "Configure Wifi", wifi_setup_activate);
LCDML_add         (13, LCDML_0_2, 2, "< Back", lcdml_menu_back);
LCDML_addAdvanced (14, LCDML_0, 3, always_false, "screensaver", lcdml_screensaver, 0, _LCDML_TYPE_default);

// menu element count - last element id
// this value must be the same as the last menu element
#define _LCDML_DISP_cnt    14

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

void settings(uint8_t param) {
    if (LCDML.FUNC_setup()) {
        // resolve some of the menu macro magic to end up with this line
        LCDMenuLib2_menu *current = LCDML.MENU_getCurrentObj()->getChild(param);
        menu_buffer.lcd_print(0, g_LCDML_DISP_lang_lcdml_table[current->getID()]);

        switch (param) {
            case 0:
                menu_buffer.lcd_print(1, WiFi.SSID().c_str());
                break;
            case 1:
//                if (wifiManager != nullptr) {
//                    menu_buffer.lcd_print_P(1, PSTR("Retry: %d"), wifiManager->connectionRetries);
//                } else {
                menu_buffer.lcd_print_P(1, PSTR("Retry unknown"));
//                }
                break;
            case 2:
                menu_buffer.lcd_print(1, WiFi.localIP().toString().c_str());
                break;
            case 3:
                menu_buffer.lcd_print(1, WiFi.gatewayIP().toString().c_str());
                break;
            case 4:
                menu_buffer.lcd_print(1, WiFi.dnsIP().toString().c_str());
                break;
            case 5:
                menu_buffer.lcd_print(1, WiFi.hostname().c_str());
                break;
            case 6:
                menu_buffer.lcd_print_P(1, PSTR("LED: %02x"), display->led);
                break;
            default:
                menu_buffer.lcd_print_P(1, PSTR("unknown"));
                break;
        }
        LCDML.FUNC_setLoopInterval(100);
    }

    if (LCDML.FUNC_loop()) {
        if (LCDML.BT_checkAny()) {
            LCDML.FUNC_goBackToMenu();
        }
    }
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
    LCDML.SCREEN_enable(lcdml_screensaver, 30000ul);

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
                        // resolve some of the menu macro magic to end up with this line
                        menu_buffer.lcd_print_P(n, " %s", g_LCDML_DISP_lang_lcdml_table[tmp->getID()]);
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

uint8_t sb_col = 0;
uint8_t sb_row = 0;
uint8_t count = 0;

void lcdml_screensaver(uint8_t param) {
    if (LCDML.FUNC_setup()) {
        menu_buffer.clear();
        menu_buffer.text[0][0] = '.';
        menu_buffer.request_render();

        LCDML.FUNC_setLoopInterval(100);
    }

    if (LCDML.FUNC_loop()) {
        if (LCDML.BT_checkAny()) {
            LCDML.FUNC_goBackToMenu();
        }

        if (++count == 10) {
            count = 0;
            menu_buffer.text[sb_row][sb_col] = ' ';
            sb_row++;
            if (sb_row == DISPLAY_ROWS) sb_row = 0;
            sb_col++;
            if (sb_col == DISPLAY_COLS) sb_col = 0;
            menu_buffer.text[sb_row][sb_col] = '.';
            menu_buffer.request_render();
        }
    }

    if (LCDML.FUNC_close()) {
        // The screensaver goes to the root menu
        menu_buffer.clear();
        LCDML.MENU_goRoot();
    }
}


void lcdml_menu_back(uint8_t param) {
    if (LCDML.FUNC_setup()) {
        // end function and go an layer back
        LCDML.FUNC_goBackToMenu(1);      // leave this function and go a layer back
    }
}
