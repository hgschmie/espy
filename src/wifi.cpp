/* -*- mode: C++; -*-
 *
 * Wifi Code
 */

#include <ESP8266WiFi.h>
#include <CustomWifiManager.h>

#include <espy.h>

AsyncWebServer server(80);

EspyDisplayBuffer wifi_buf;
CustomWiFiManager *wifiManager = nullptr;

CustomWiFiManagerParameter mqtt_server("server", "mqtt server", "mqtt.intermeta.com", 40);

void wifi_scan_task() {
    if (wifiManager != nullptr) {
        wifi_buf.leds[0] = led_state::FAST;
        wifiManager->scanNetworks();
        wifi_buf.leds[0] = led_state::OFF;
    }
}

void wifi_connect_task() {
    EspyDisplayBuffer *current_buffer = display->current;

    if (wifiManager != nullptr) {
        wifiManager->connectTask();
        if (WiFi.status() == WL_CONNECTED) {
            if (current_buffer != nullptr) {
                current_buffer->leds[1] = led_state::OFF;
                current_buffer->leds[2] = led_state::ON;
                current_buffer->request_render();
            }
        } else {
            if (current_buffer != nullptr) {
                LED_TOGGLE(current_buffer, 1);
            }
        }
    }
}

Task wifiScanTask(10000, TASK_FOREVER, &wifi_scan_task);
Task wifiConnectTask(1000, TASK_FOREVER, &wifi_connect_task);

void wifi_setup(Scheduler &scheduler) {
    scheduler.addTask(wifiScanTask);
    scheduler.addTask(wifiConnectTask);

    wifiManager = new CustomWiFiManager(&server);
    wifiManager->addParameter(&mqtt_server);

    wifiConnectTask.enable();
}

void wifi_config_mode() {
    CustomWiFiManager::resetSettings();

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
        wifi_buf.leds[3] = led_state::ON;

        LCDML.FUNC_setLoopInterval(100);

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

        if (wifiManager->configTask()) {
            LCDML.FUNC_goBackToMenu();
        }

    }

    if (LCDML.FUNC_close()) {

        wifi_connect_mode();
        // restore the menu buffer for display
        display->display(&menu_buffer);
    }
}
