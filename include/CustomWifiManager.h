/**************************************************************
   WiFiManager is a library for the ESP8266/Arduino platform
   (https://github.com/esp8266/Arduino) to enable easy
   configuration and reconfiguration of WiFi credentials using a Captive Portal
   inspired by:
   http://www.esp8266.com/viewtopic.php?f=29&t=2520
   https://github.com/chriscook8/esp-arduino-apboot
   https://github.com/esp8266/Arduino/tree/esp8266/hardware/esp8266com/esp8266/libraries/DNSServer/examples/CaptivePortalAdvanced
   Built by AlexT https://github.com/tzapu
   Ported to Async Web Server by https://github.com/alanswx
   Licensed under MIT license
 **************************************************************/

#ifndef _CUSTOM_WIFI_MANAGER_H_
#define _CUSTOM_WIFI_MANAGER_H_

#if defined(ESP8266)

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

#else
#include <WiFi.h>
#include "esp_wps.h"
#define ESP_WPS_MODE WPS_TYPE_PBC
#endif

#include <ESPAsyncWebServer.h>

#include <memory>

// fix crash on ESP32 (see https://github.com/alanswx/ESPAsyncWiFiManager/issues/44)
#if defined(ESP8266)
typedef int8_t wifi_ssid_count_t;
#else
typedef int16_t wifi_ssid_count_t;
#endif

#if defined(ESP8266)
extern "C" {
#include "user_interface.h"
}
#else
#include <rom/rtc.h>
#endif

const char WFM_HTTP_HEAD[] PROGMEM = R"(<!DOCTYPE html><html lang="en"><head><meta name="viewport" content="width=device-width, initial-scale=1, user-scalable=no"/><title>{v}</title>)";
const char HTTP_STYLE[] PROGMEM = R"(<style>.c{text-align: center;} div,input{padding:5px;font-size:1em;} input{width:95%;} body{text-align: center;font-family:verdana;} button{border:0;border-radius:0.3rem;background-color:#1fa3ec;color:#fff;line-height:2.4rem;font-size:1.2rem;width:100%;} .q{float: right;width: 64px;text-align: right;} .l{background: url(\"data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAACAAAAAgCAMAAABEpIrGAAAALVBMVEX///8EBwfBwsLw8PAzNjaCg4NTVVUjJiZDRUUUFxdiZGSho6OSk5Pg4eFydHTCjaf3AAAAZElEQVQ4je2NSw7AIAhEBamKn97/uMXEGBvozkWb9C2Zx4xzWykBhFAeYp9gkLyZE0zIMno9n4g19hmdY39scwqVkOXaxph0ZCXQcqxSpgQpONa59wkRDOL93eAXvimwlbPbwwVAegLS1HGfZAAAAABJRU5ErkJggg==\") no-repeat left center;background-size: 1em;}</style>)";
const char HTTP_SCRIPT[] PROGMEM = R"(<script>function c(l){document.getElementById('s').value=l.innerText||l.textContent;document.getElementById('p').focus();}</script>)";
const char HTTP_HEAD_END[] PROGMEM = R"(</head><body><div style='text-align:left;display:inline-block;min-width:260px;'>)";
const char HTTP_PORTAL_OPTIONS[] PROGMEM = R"(<form action="/wifi" method="get"><button>Configure WiFi</button></form><br/><form action="/0wifi" method="get"><button>Configure WiFi (No Scan)</button></form><br/><form action="/i" method="get"><button>Info</button></form><br/><form action="/r" method="post"><button>Reset</button></form>)";
const char HTTP_ITEM[] PROGMEM = R"(<div><a href='#p' onclick='c(this)'>{v}</a>&nbsp;<span class='q {i}'>{r}%</span></div>)";
const char HTTP_FORM_START[] PROGMEM = R"(<form method='get' action='wifisave'><input id='s' name='s' length=32 placeholder='SSID'><br/><input id='p' name='p' length=64 type='password' placeholder='password'><br/>)";
const char HTTP_FORM_PARAM[] PROGMEM = R"(<br/><input id='{i}' name='{n}' length={l} placeholder='{p}' value='{v}' {c}>)";
const char HTTP_FORM_END[] PROGMEM = R"(<br/><button type='submit'>save</button></form>)";
const char HTTP_SCAN_LINK[] PROGMEM = R"(<br/><div class="c"><a href="/wifi">Scan</a></div>)";
const char HTTP_SAVED[] PROGMEM = R"(<div>Credentials Saved<br />Trying to connect ESP to network.<br />If it fails reconnect to AP to try again</div>)";
const char HTTP_END[] PROGMEM = R"(</div></body></html>)";

#define WIFI_MANAGER_MAX_PARAMS 10
#define WIFI_MANAGER_MAX_RETRIES 30

class CustomWiFiManagerParameter {
public:
    explicit CustomWiFiManagerParameter(const char *custom);

    CustomWiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length);

    CustomWiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom);

    const char *getID();

    const char *getValue();

    const char *getPlaceholder();

    int getValueLength();

    const char *getCustomHTML();

private:
    const char *_id;
    const char *_placeholder;
    char *_value;
    int _length;
    const char *_customHTML;

    void init(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom);

    friend class CustomWiFiManager;
};


class WiFiResult {
public:
    bool duplicate;
    String SSID;
    uint8_t encryptionType;
    int32_t RSSI;
    uint8_t *BSSID;
    int32_t channel;
    bool isHidden;

    WiFiResult() {
    }

};

class CustomWiFiManager {
public:
    unsigned int retry = 0;

    explicit CustomWiFiManager(AsyncWebServer *server);

    void scan();

    static String infoAsString();

    //if you want to always start the config portal, without trying to connect first
    void enableConfigPortal(char const *apName, char const *apPassword = nullptr);

    bool configTask();

    void connectTask();

    void scanNetworks();

    static void resetSettings();

    //defaults to not showing anything under 8% signal quality if called
    void setMinimumSignalQuality(int quality = 8);

    //called when settings have been changed and connection was successful
    void setSaveConfigCallback(void (*func)());

    //adds a custom parameter
    void addParameter(CustomWiFiManagerParameter *p);

    //if this is set, customise style
    void setCustomHeadElement(const char *element);

    //sets a custom element to add to options page
    void setCustomOptionsElement(const char *element);

private:
    AsyncWebServer *server;

    void setupConfigPortal();

    String infoPage = "";
    wl_status_t wifiStatus = WL_NO_SHIELD;
    const char *_apName = "no-net";
    const char *_apPassword = nullptr;

    String _ssid = "";
    String _pass = "";

    int _paramsCount = 0;
    int _minimumQuality = -1;

    const char *_customHeadElement = "";
    const char *_customOptionsElement = "";

    int status = WL_IDLE_STATUS;

    int connectWifi(const String &ssid, const String &pass);

    static void disableWifi();

    void setInfo();

    String networkListAsString();

    void handleRoot(AsyncWebServerRequest *);

    void handleWifi(AsyncWebServerRequest *, boolean scan);

    void handleWifiSave(AsyncWebServerRequest *);

    void handleInfo(AsyncWebServerRequest *);

    void handleReset(AsyncWebServerRequest *);

    void handleNotFound(AsyncWebServerRequest *);

    static boolean captivePortal(AsyncWebServerRequest *);

    //helpers


    boolean connect;

    WiFiResult *wifiSSIDs;
    wifi_ssid_count_t wifiSSIDCount;

    void (*_savecallback)() = nullptr;

    CustomWiFiManagerParameter *_params[WIFI_MANAGER_MAX_PARAMS];

    static int getRSSIasQuality(int RSSI);

    static boolean isIp(const String &str);

    static String toStringIp(const IPAddress &ip);
};

#endif
