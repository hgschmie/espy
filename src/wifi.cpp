/* -*- mode: C++; -*-
 *
 * Wifi Code
 */

#include <ESP8266WiFi.h>
#include <CustomAsyncWifiManager.h>

#include <espy.h>

AsyncWebServer server(80);

EspyDisplayBuffer wifi_buf;
AsyncWiFiManager *wifiManager = nullptr;

void wifi_scan_task() {
    if (wifiManager != nullptr) {
        wifiManager->scanNetworks();
        LED_TOGGLE(wifi_buf, 0);
    }
}

Task wifiScanTask(10000, TASK_FOREVER, &wifi_scan_task);

void portal_setup(Scheduler & scheduler) {
    scheduler.addTask(wifiScanTask);
}
void wifi_scan_enable() {
    wifiScanTask.enable();
}

void wifi_scan_disable() {
    wifiScanTask.disable();
}

void wifi_setup_activate(uint8_t param) {
    if (LCDML.FUNC_setup()) {
        display->display(&wifi_buf);
        wifi_buf.leds[3] = led_state::FAST;

        wifiManager = new AsyncWiFiManager(&server);
        wifiManager->setTimeout(0);

        LCDML.FUNC_setLoopInterval(100);

        WiFi.begin();
        wifiManager->resetSettings();
        wifiManager->enableConfigPortal("NuclearDevice");
        dns_enable();
        wifi_scan_enable();

        wifi_buf.lcd_print_P(0, PSTR("%s"), WiFi.softAPSSID().c_str());
        wifi_buf.lcd_print_P(1, PSTR("%s"), WiFi.softAPIP().toString().c_str());
    }

    if (LCDML.FUNC_loop()) {
        if (LCDML.BT_checkAny()) {
            LCDML.FUNC_goBackToMenu();
        }
        // can't have screen blanker here.
        LCDML.SCREEN_resetTimer();
        LED_TOGGLE(wifi_buf, 2);

        if(wifiManager->configTask()) {
            LCDML.FUNC_goBackToMenu();
        }

    }

    if (LCDML.FUNC_close()) {
        wifi_scan_disable();
        dns_disable();
        // restore the menu buffer for display
        display->display(&menu_buffer);
    }
}
