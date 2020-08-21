/**************************************************************
   AsyncWiFiManager is a library for the ESP8266/Arduino platform
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

#include "CustomAsyncWifiManager.h"

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *custom) {
    _id = nullptr;
    _placeholder = nullptr;
    _length = 0;
    _value = nullptr;

    _customHTML = custom;
}

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length) {
    init(id, placeholder, defaultValue, length, "");
}

AsyncWiFiManagerParameter::AsyncWiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom) {
    init(id, placeholder, defaultValue, length, custom);
}

void AsyncWiFiManagerParameter::init(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom) {
    _id = id;
    _placeholder = placeholder;
    _length = length;
    _value = new char[length + 1];
    for (int i = 0; i < length; i++) {
        _value[i] = 0;
    }
    if (defaultValue != nullptr) {
        strncpy(_value, defaultValue, length);
    }

    _customHTML = custom;
}

const char *AsyncWiFiManagerParameter::getValue() {
    return _value;
}

const char *AsyncWiFiManagerParameter::getID() {
    return _id;
}

const char *AsyncWiFiManagerParameter::getPlaceholder() {
    return _placeholder;
}

int AsyncWiFiManagerParameter::getValueLength() {
    return _length;
}

const char *AsyncWiFiManagerParameter::getCustomHTML() {
    return _customHTML;
}

AsyncWiFiManager::AsyncWiFiManager(AsyncWebServer *server) : server(server) {
    wifiSSIDs = nullptr;
    wifiSSIDscan = true;
    _modeless = false;
}

void AsyncWiFiManager::addParameter(AsyncWiFiManagerParameter *p) {
    _params[_paramsCount] = p;
    _paramsCount++;
}

void AsyncWiFiManager::setupConfigPortal() {
    server->reset();

    if (_apPassword != nullptr) {
        if (strlen(_apPassword) < 8 || strlen(_apPassword) > 63) {
            // fail passphrase to short or long!
            _apPassword = nullptr;
        }
    }

    if (_apPassword != nullptr) {
        WiFi.softAP(_apName, _apPassword);//password option
    } else {
        WiFi.softAP(_apName);
    }

    delay(500); // Without delay I've seen the IP address blank

    setInfo();

    /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
    server->on("/", std::bind(&AsyncWiFiManager::handleRoot, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    server->on("/wifi", std::bind(&AsyncWiFiManager::handleWifi, this, std::placeholders::_1, true)).setFilter(ON_AP_FILTER);
    server->on("/0wifi", std::bind(&AsyncWiFiManager::handleWifi, this, std::placeholders::_1, false)).setFilter(ON_AP_FILTER);
    server->on("/wifisave", std::bind(&AsyncWiFiManager::handleWifiSave, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    server->on("/i", std::bind(&AsyncWiFiManager::handleInfo, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    server->on("/r", std::bind(&AsyncWiFiManager::handleReset, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    server->on("/fwlink", std::bind(&AsyncWiFiManager::handleRoot, this, std::placeholders::_1)).setFilter(
            ON_AP_FILTER);  //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    server->onNotFound(std::bind(&AsyncWiFiManager::handleNotFound, this, std::placeholders::_1));
    server->begin(); // Web server start
}

static const char HEX_CHAR_ARRAY[17] = "0123456789ABCDEF";

/**
* convert char array (hex values) to readable string by seperator
* buf:           buffer to convert
* length:        data length
* strSeperator   seperator between each hex value
* return:        formated value as String
*/
static String byteToHexString(uint8_t *buf, uint8_t length, String strSeperator = "-") {
    String dataString = "";
    for (uint8_t i = 0; i < length; i++) {
        byte v = buf[i] / 16;
        byte w = buf[i] % 16;
        if (i > 0) {
            dataString += strSeperator;
        }
        dataString += String(HEX_CHAR_ARRAY[v]);
        dataString += String(HEX_CHAR_ARRAY[w]);
    }
    dataString.toUpperCase();
    return dataString;
} // byteToHexString

#if !defined(ESP8266)
String getESP32ChipID() {
  uint64_t chipid;
  chipid=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  int chipid_size = 6;
  uint8_t chipid_arr[chipid_size];
  for (uint8_t i=0; i < chipid_size; i++) {
    chipid_arr[i] = (chipid >> (8 * i)) & 0xff;
  }
  return byteToHexString(chipid_arr, chipid_size, "");
}
#endif

boolean AsyncWiFiManager::autoConnect(unsigned long maxConnectRetries, unsigned long retryDelayMs) {
    String ssid = "ESP";
#if defined(ESP8266)
    ssid += String(ESP.getChipId());
#else
    ssid += getESP32ChipID();
#endif
    return autoConnect(ssid.c_str(), nullptr);
}

boolean AsyncWiFiManager::autoConnect(char const *apName, char const *apPassword, unsigned long maxConnectRetries, unsigned long retryDelayMs) {

    // attempt to connect; should it fail, fall back to AP
    WiFi.mode(WIFI_STA);

    for (unsigned long tryNumber = 0; tryNumber < maxConnectRetries; tryNumber++) {
        if (connectWifi("", "") == WL_CONNECTED) {
            //connected
            return true;
        }

        if (tryNumber + 1 < maxConnectRetries) {

            // we might connect during the delay
            unsigned long restDelayMs = retryDelayMs;
            while (restDelayMs != 0) {
                if (WiFi.status() == WL_CONNECTED) {
                    return true;
                }
                unsigned long thisDelay = std::min(restDelayMs, 100ul);
                delay(thisDelay);
                restDelayMs -= thisDelay;
            }

        }
    }


    // TODO return startConfigPortal(apName, apPassword);
}


String AsyncWiFiManager::networkListAsString() {
    String pager;
    //display networks in page
    for (int i = 0; i < wifiSSIDCount; i++) {
        if (wifiSSIDs[i].duplicate == true) continue; // skip dups
        int quality = getRSSIasQuality(wifiSSIDs[i].RSSI);

        if (_minimumQuality == -1 || _minimumQuality < quality) {
            String item = FPSTR(HTTP_ITEM);
            String rssiQ;
            rssiQ += quality;
            item.replace("{v}", wifiSSIDs[i].SSID);
            item.replace("{r}", rssiQ);
#if defined(ESP8266)
            if (wifiSSIDs[i].encryptionType != ENC_TYPE_NONE) {
#else
                if (wifiSSIDs[i].encryptionType != WIFI_AUTH_OPEN) {
#endif
                item.replace("{i}", "l");
            } else {
                item.replace("{i}", "");
            }
            pager += item;

        }
    }
    return pager;
}

void AsyncWiFiManager::scan() {
    if (wifiSSIDscan) {
        delay(100);
        wifi_ssid_count_t n = WiFi.scanNetworks();

        // error
        if (n < 0) {
            return;
        }

        if (wifiSSIDscan) {
            /* WE SHOULD MOVE THIS IN PLACE ATOMICALLY */
            if (wifiSSIDs) delete[] wifiSSIDs;
            wifiSSIDs = new WiFiResult[n];
            wifiSSIDCount = n;

            for (wifi_ssid_count_t i = 0; i < n; i++) {
                wifiSSIDs[i].duplicate = false;

#if defined(ESP8266)
                bool res = WiFi.getNetworkInfo(i, wifiSSIDs[i].SSID, wifiSSIDs[i].encryptionType, wifiSSIDs[i].RSSI, wifiSSIDs[i].BSSID,
                                               wifiSSIDs[i].channel, wifiSSIDs[i].isHidden);
#else
                bool res=WiFi.getNetworkInfo(i, wifiSSIDs[i].SSID, wifiSSIDs[i].encryptionType, wifiSSIDs[i].RSSI, wifiSSIDs[i].BSSID, wifiSSIDs[i].channel);
#endif
            }


            // RSSI SORT

            // old sort
            for (int i = 0; i < n; i++) {
                for (int j = i + 1; j < n; j++) {
                    if (wifiSSIDs[j].RSSI > wifiSSIDs[i].RSSI) {
                        std::swap(wifiSSIDs[i], wifiSSIDs[j]);
                    }
                }
            }


            // remove duplicates ( must be RSSI sorted )
            String cssid;
            for (int i = 0; i < n; i++) {
                if (wifiSSIDs[i].duplicate == true) continue;
                cssid = wifiSSIDs[i].SSID;
                for (int j = i + 1; j < n; j++) {
                    if (cssid == wifiSSIDs[j].SSID) {
                        wifiSSIDs[j].duplicate = true; // set dup aps to NULL
                    }
                }
            }
        }
    }
}

void AsyncWiFiManager::loop() {
    criticalLoop();
}

void AsyncWiFiManager::setInfo() {
    if (needInfo) {
        pager = infoAsString();
        wifiStatus = WiFi.status();
        needInfo = false;
    }
}

/**
 * Anything that accesses WiFi, ESP or EEPROM goes here
 */
void AsyncWiFiManager::criticalLoop() {
    if (_modeless) {

        if (scannow == -1 || millis() > scannow + 60000u) {
            scan();
            scannow = millis();
        }
        if (connect) {
            connect = false;

            // using user-provided  _ssid, _pass in place of system-stored ssid and pass
            if (connectWifi(_ssid, _pass) == WL_CONNECTED) {
                //connected
                // alanswx - should we have a config to decide if we should shut down AP?
                // WiFi.mode(WIFI_STA);
                //notify that configuration has changed and any optional parameters should be saved
                if (_savecallback != NULL) {
                    //todo: check if any custom parameters actually exist, and check if they really changed maybe
                    _savecallback();
                }

                return;
            }

        }
    }
}

void AsyncWiFiManager::enableConfigPortal(char const *apName, char const *apPassword) {
    //setup AP
    WiFi.mode(WIFI_AP_STA);

    _apName = apName;
    _apPassword = apPassword;

    connect = false;
    setupConfigPortal();
}

void AsyncWiFiManager::scanNetworks() {
#if defined(ESP8266)
    // we might still be connecting, so that has to stop for scanning
    ETS_UART_INTR_DISABLE ();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE ();
#else
    WiFi.disconnect (false);
#endif
    scan();
    WiFi.begin(); // try to reconnect to AP
}

// do a single iteration through the configuration loop
bool AsyncWiFiManager::configTask() {

    // attempts to reconnect were successful
    if (WiFi.status() == WL_CONNECTED) {
        //connected
        WiFi.mode(WIFI_STA);
        //notify that configuration has changed and any optional parameters should be saved
        if (_savecallback != nullptr) {
            //todo: check if any custom parameters actually exist, and check if they really changed maybe
            _savecallback();
        }
        server->reset();
    } else if (connect) {
        connect = false;
        delay(2000);

        // using user-provided  _ssid, _pass in place of system-stored ssid and pass
        if (connectWifi(_ssid, _pass) == WL_CONNECTED) {
            //connected
            WiFi.mode(WIFI_STA);
            //notify that configuration has changed and any optional parameters should be saved
            if (_savecallback != nullptr) {
                //todo: check if any custom parameters actually exist, and check if they really changed maybe
                _savecallback();
            }
            server->reset();
        }
    }
    return WiFi.status() == WL_CONNECTED;
}


int AsyncWiFiManager::connectWifi(String ssid, String pass) {

    //check if we have ssid and pass and force those, if not, try with last saved values
    if (ssid != "") {
#if defined(ESP8266)
        //trying to fix connection in progress hanging
        ETS_UART_INTR_DISABLE();
        wifi_station_disconnect();
        ETS_UART_INTR_ENABLE();
#else
        WiFi.disconnect(false);
#endif

        WiFi.begin(ssid.c_str(), pass.c_str());
    } else {
        if (WiFi.SSID().length() > 0) {
#if defined(ESP8266)
            //trying to fix connection in progress hanging
            ETS_UART_INTR_DISABLE();
            wifi_station_disconnect();
            ETS_UART_INTR_ENABLE();
#else
            WiFi.disconnect(false);
#endif

            WiFi.begin();
        } else {
            WiFi.begin();
        }
    }

    int connRes = waitForConnectResult();
    //not connected, WPS enabled, no pass - first attempt
#ifdef NO_EXTRA_4K_HEAP
    if (_tryWPS && connRes != WL_CONNECTED && pass == "") {
      startWPS();
      //should be connected at the end of WPS
      connRes = waitForConnectResult();
    }
#endif
    needInfo = true;
    setInfo();
    return connRes;
}

uint8_t AsyncWiFiManager::waitForConnectResult() {
    if (_connectTimeout == 0) {
        return WiFi.waitForConnectResult();
    } else {
        unsigned long start = millis();
        boolean keepConnecting = true;
        uint8_t status;
        while (keepConnecting) {
            status = WiFi.status();
            if (millis() > start + _connectTimeout) {
                keepConnecting = false;
            }
            if (status == WL_CONNECTED || status == WL_CONNECT_FAILED) {
                keepConnecting = false;
            }
            delay(100);
        }
        return status;
    }
}

#ifdef NO_EXTRA_4K_HEAP
void AsyncWiFiManager::startWPS() {
  DEBUG_WM("START WPS");
#if defined(ESP8266)
  WiFi.beginWPSConfig();
#else
  //esp_wps_config_t config = WPS_CONFIG_INIT_DEFAULT(ESP_WPS_MODE);
  esp_wps_config_t config = {};
  config.wps_type = ESP_WPS_MODE;
  config.crypto_funcs = &g_wifi_default_wps_crypto_funcs;
  strcpy(config.factory_info.manufacturer,"ESPRESSIF");  
  strcpy(config.factory_info.model_number, "ESP32");  
  strcpy(config.factory_info.model_name, "ESPRESSIF IOT");  
  strcpy(config.factory_info.device_name,"ESP STATION");  

  esp_wifi_wps_enable(&config);
  esp_wifi_wps_start(0);
#endif
  DEBUG_WM("END WPS");

}
#endif

String AsyncWiFiManager::getConfigPortalSSID() {
    return _apName;
}

void AsyncWiFiManager::resetSettings() {
    WiFi.disconnect(true);
}

void AsyncWiFiManager::setTimeout(unsigned long seconds) {
    setConfigPortalTimeout(seconds);
}

void AsyncWiFiManager::setConfigPortalTimeout(unsigned long seconds) {
    _configPortalTimeout = seconds * 1000;
}

void AsyncWiFiManager::setConnectTimeout(unsigned long seconds) {
    _connectTimeout = seconds * 1000;
}

void AsyncWiFiManager::setMinimumSignalQuality(int quality) {
    _minimumQuality = quality;
}

/** Handle root or redirect to captive portal */
void AsyncWiFiManager::handleRoot(AsyncWebServerRequest *request) {
    // AJS - maybe we should set a scan when we get to the root???
    // and only scan on demand? timer + on demand? plus a link to make it happen?
    scannow = -1;
    if (captivePortal(request)) { // If captive portal redirect instead of displaying the page.
        return;
    }

    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Options");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _customHeadElement;
    page += FPSTR(HTTP_HEAD_END);
    page += "<h1>";
    page += _apName;
    page += "</h1>";
    page += F("<h3>AsyncWiFiManager</h3>");
    page += FPSTR(HTTP_PORTAL_OPTIONS);
    page += _customOptionsElement;
    page += FPSTR(HTTP_END);

    request->send(200, "text/html", page);

}

/** Wifi config page handler */
void AsyncWiFiManager::handleWifi(AsyncWebServerRequest *request, boolean scan) {
    scannow = -1;

    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Config ESP");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _customHeadElement;
    page += FPSTR(HTTP_HEAD_END);

    if (scan) {
        wifiSSIDscan = false;
        if (wifiSSIDCount == 0) {
            page += F("No networks found. Refresh to scan again.");
        } else {


            //display networks in page
            String pager = networkListAsString();

            page += pager;
            page += "<br/>";
        }

    }
    wifiSSIDscan = true;

    page += FPSTR(HTTP_FORM_START);
    char parLength[2];
    // add the extra parameters to the form
    for (int i = 0; i < _paramsCount; i++) {
        if (_params[i] == nullptr) {
            break;
        }

        String pitem = FPSTR(HTTP_FORM_PARAM);
        if (_params[i]->getID() != nullptr) {
            pitem.replace("{i}", _params[i]->getID());
            pitem.replace("{n}", _params[i]->getID());
            pitem.replace("{p}", _params[i]->getPlaceholder());
            snprintf(parLength, 2, "%d", _params[i]->getValueLength());
            pitem.replace("{l}", parLength);
            pitem.replace("{v}", _params[i]->getValue());
            pitem.replace("{c}", _params[i]->getCustomHTML());
        } else {
            pitem = _params[i]->getCustomHTML();
        }

        page += pitem;
    }
    if (_params[0] != nullptr) {
        page += "<br/>";
    }


    page += FPSTR(HTTP_FORM_END);
    page += FPSTR(HTTP_SCAN_LINK);

    page += FPSTR(HTTP_END);

    request->send(200, "text/html", page);
}

/** Handle the WLAN save form and redirect to WLAN config page again */
void AsyncWiFiManager::handleWifiSave(AsyncWebServerRequest *request) {

    //SAVE/connect here
    needInfo = true;
    _ssid = request->arg("s").c_str();
    _pass = request->arg("p").c_str();

    //parameters
    for (int i = 0; i < _paramsCount; i++) {
        if (_params[i] == nullptr) {
            break;
        }
        //read parameter
        String value = request->arg(_params[i]->getID()).c_str();
        //store it in array
        value.toCharArray(_params[i]->_value, _params[i]->_length);
    }

    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Credentials Saved");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _customHeadElement;
    page += F("<meta http-equiv=\"refresh\" content=\"5; url=/i\">");
    page += FPSTR(HTTP_HEAD_END);
    page += FPSTR(HTTP_SAVED);
    page += FPSTR(HTTP_END);

    request->send(200, "text/html", page);

    connect = true; //signal ready to connect/reset
}

/** Handle the info page */
String AsyncWiFiManager::infoAsString() {
    String page;
    page += F("<dt>Chip ID</dt><dd>");
#if defined(ESP8266)
    page += ESP.getChipId();
#else
    page += getESP32ChipID();
#endif
    page += F("</dd>");
    page += F("<dt>Flash Chip ID</dt><dd>");
#if defined(ESP8266)
    page += ESP.getFlashChipId();
#else
    page += F("N/A for ESP32");
#endif
    page += F("</dd>");
    page += F("<dt>IDE Flash Size</dt><dd>");
    page += ESP.getFlashChipSize();
    page += F(" bytes</dd>");
    page += F("<dt>Real Flash Size</dt><dd>");
#if defined(ESP8266)
    page += ESP.getFlashChipRealSize();
#else
    page += F("N/A for ESP32");
#endif
    page += F(" bytes</dd>");
    page += F("<dt>Soft AP IP</dt><dd>");
    page += WiFi.softAPIP().toString();
    page += F("</dd>");
    page += F("<dt>Soft AP MAC</dt><dd>");
    page += WiFi.softAPmacAddress();
    page += F("</dd>");
    page += F("<dt>Station SSID</dt><dd>");
    page += WiFi.SSID();
    page += F("</dd>");
    page += F("<dt>Station IP</dt><dd>");
    page += WiFi.localIP().toString();
    page += F("</dd>");
    page += F("<dt>Station MAC</dt><dd>");
    page += WiFi.macAddress();
    page += F("</dd>");
    page += F("</dl>");
    return page;
}

void AsyncWiFiManager::handleInfo(AsyncWebServerRequest *request) {
    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Info");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _customHeadElement;
    if (connect == true)
        page += F("<meta http-equiv=\"refresh\" content=\"5; url=/i\">");
    page += FPSTR(HTTP_HEAD_END);
    page += F("<dl>");
    if (connect == true) {
        page += F("<dt>Trying to connect</dt><dd>");
        page += wifiStatus;
        page += F("</dd>");
    }

    page += pager;
    page += FPSTR(HTTP_END);

    request->send(200, "text/html", page);
}

/** Handle the reset page */
void AsyncWiFiManager::handleReset(AsyncWebServerRequest *request) {
    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Info");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _customHeadElement;
    page += FPSTR(HTTP_HEAD_END);
    page += F("Module will reset in a few seconds.");
    page += FPSTR(HTTP_END);
    request->send(200, "text/html", page);

    delay(5000);
#if defined(ESP8266)
    ESP.reset();
#else
    ESP.restart();
#endif
    delay(2000);
}

void AsyncWiFiManager::handleNotFound(AsyncWebServerRequest *request) {
    if (captivePortal(request)) { // If captive portal redirect instead of displaying the error page.
        return;
    }
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += request->url();
    message += "\nMethod: ";
    message += (request->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += request->args();
    message += "\n";

    for (uint8_t i = 0; i < request->args(); i++) {
        message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
    }
    AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", message);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
    request->send(response);
}


/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean AsyncWiFiManager::captivePortal(AsyncWebServerRequest *request) {
    if (!isIp(request->host())) {
        AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
        response->addHeader("Location", String("http://") + toStringIp(request->client()->localIP()));
        request->send(response);
        return true;
    }
    return false;
}

//start up save config callback
void AsyncWiFiManager::setSaveConfigCallback(void (*func)(void)) {
    _savecallback = func;
}

//sets a custom element to add to head, like a new style tag
void AsyncWiFiManager::setCustomHeadElement(const char *element) {
    _customHeadElement = element;
}

//sets a custom element to add to options page
void AsyncWiFiManager::setCustomOptionsElement(const char *element) {
    _customOptionsElement = element;
}

int AsyncWiFiManager::getRSSIasQuality(int RSSI) {
    int quality = 0;

    if (RSSI <= -100) {
        quality = 0;
    } else if (RSSI >= -50) {
        quality = 100;
    } else {
        quality = 2 * (RSSI + 100);
    }
    return quality;
}

/** Is this an IP? */
boolean AsyncWiFiManager::isIp(String str) {
    for (unsigned int i = 0; i < str.length(); i++) {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9')) {
            return false;
        }
    }
    return true;
}

/** IP to String? */
String AsyncWiFiManager::toStringIp(IPAddress ip) {
    String res = "";
    for (int i = 0; i < 3; i++) {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}
