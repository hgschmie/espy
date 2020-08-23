/* -*- mode: C++; -*-
 *
 * Wifi Code
 */

#include <ESP8266WiFi.h>
#include <CustomWifiManager.h>

#include <espy.h>

AsyncWebServer server(80);

EspyDisplayBuffer wifi_buf("wifi");
CustomWiFiManager *wifiManager = nullptr;

CustomWiFiManagerParameter mqtt_server("server", "mqtt server", "mqtt.intermeta.com", 40);

void wifi_scan_task() {
    wifi_buf.leds[0] = led_state::ON;
    display->refresh(); // needs a refresh as the scan is blocking

    if (wifiManager != nullptr) {
        wifiManager->scanNetworkTask();
    }

    wifi_buf.leds[0] = led_state::OFF;
    display->refresh(); // needs a refresh as the scan is blocking
}

//
// LED 2 flashes while connecting, LED 3 turns on when connected
//
void wifi_connect_task() {
    if (wifiManager != nullptr) {
        wifiManager->connectTask();

        if (WiFi.status() == WL_CONNECTED) {
            menu_buffer.leds[2] = led_state::OFF;
            menu_buffer.leds[3] = led_state::ON;
        } else {
            menu_buffer.leds[3] = led_state::OFF;
            LED_TOGGLE(&menu_buffer, 2);
        }
    }
}

Task wifiScanTask(10000, TASK_FOREVER, &wifi_scan_task);
Task wifiConnectTask(WIFI_MANAGER_CONNECTION_TASK_TIME_MS, TASK_FOREVER, &wifi_connect_task);

void wifi_setup(Scheduler &scheduler) {
    scheduler.addTask(wifiScanTask);
    scheduler.addTask(wifiConnectTask);

    wifiManager = new CustomWiFiManager(&server);
    wifiManager->addParameter(&mqtt_server);

    wifiConnectTask.enable();
}

void wifi_config_mode() {
    wifiManager->resetSettings();

    server.reset();
    wifiManager->enableConfigPortal("NuclearDevice");
    dns_enable();

    wifiConnectTask.disable();
    wifiScanTask.enable();
}

void wifi_connect_mode() {
    wifiScanTask.disable();
    wifiConnectTask.enable();

    server.reset();
    dns_disable();
}

void wifi_setup_activate(uint8_t param) {
    if (LCDML.FUNC_setup()) {
        display->display(&wifi_buf);
        wifi_buf.leds[4] = led_state::ON;

        LCDML.FUNC_setLoopInterval(WIFI_MANAGER_CONFIG_MENU_TASK_TIME_MS);

        wifi_config_mode();

        wifi_buf.lcd_print_P(0, PSTR("%s"), WiFi.softAPSSID().c_str());
        wifi_buf.lcd_print_P(1, PSTR("%s"), WiFi.softAPIP().toString().c_str());
    }

    if (LCDML.FUNC_loop()) {
        if (LCDML.BT_checkAny()) {
            LCDML.FUNC_goBackToMenu();
        }
        // can't have screen blanker here.
        LCDML.SCREEN_resetTimer();

        if (wifiManager->configPortalMenu()) {
            LCDML.FUNC_goBackToMenu();
        }

    }

    if (LCDML.FUNC_close()) {

        wifi_connect_mode();
        // restore the menu buffer for display
        display->display(&menu_buffer);
    }
}

int countdown, it;

void wifi_reset(uint8_t param) {
    if (LCDML.FUNC_setup()) {
        countdown = 6;
        it = 1000 / WIFI_MANAGER_CONFIG_MENU_TASK_TIME_MS;
        display->display(&wifi_buf);
        wifi_buf.lcd_print_P(0, PSTR("WIFI RESET!"));

        LCDML.FUNC_setLoopInterval(WIFI_MANAGER_CONFIG_MENU_TASK_TIME_MS);
    }

    if (LCDML.FUNC_loop()) {
        if (LCDML.BT_checkAny()) {
            LCDML.FUNC_goBackToMenu();
        }

        if (--it == 0) {
            it = 1000 / WIFI_MANAGER_CONFIG_MENU_TASK_TIME_MS;
            if (--countdown < 0) {
                wifi_buf.lcd_print_P(1, PSTR("Resetting"));
                wifiManager->resetSettings();
                LCDML.FUNC_goBackToMenu();
            } else {
                wifi_buf.lcd_print_P(1, PSTR("%d..."), countdown);
            }
        }
    }

    if (LCDML.FUNC_close()) {
        display->display(&menu_buffer);
    }
}