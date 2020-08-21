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


#include "CustomWifiManager.h"

CustomWiFiManagerParameter::CustomWiFiManagerParameter(const char *custom) {
    _id = nullptr;
    _placeholder = nullptr;
    _length = 0;
    _value = nullptr;

    _customHTML = custom;
}

CustomWiFiManagerParameter::CustomWiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length) {
    init(id, placeholder, defaultValue, length, "");
}

CustomWiFiManagerParameter::CustomWiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom) {
    init(id, placeholder, defaultValue, length, custom);
}

void CustomWiFiManagerParameter::init(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom) {
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

const char *CustomWiFiManagerParameter::getValue() {
    return _value;
}

const char *CustomWiFiManagerParameter::getID() {
    return _id;
}

const char *CustomWiFiManagerParameter::getPlaceholder() {
    return _placeholder;
}

int CustomWiFiManagerParameter::getValueLength() {
    return _length;
}

const char *CustomWiFiManagerParameter::getCustomHTML() {
    return _customHTML;
}

CustomWiFiManager::CustomWiFiManager(AsyncWebServer *server) : server(server) {
    wifiSSIDs = nullptr;
}

void CustomWiFiManager::addParameter(CustomWiFiManagerParameter *p) {
    _params[_paramsCount] = p;
    _paramsCount++;
}

void CustomWiFiManager::setupConfigPortal() {
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
    server->on("/", std::bind(&CustomWiFiManager::handleRoot, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    server->on("/wifi", std::bind(&CustomWiFiManager::handleWifi, this, std::placeholders::_1, true)).setFilter(ON_AP_FILTER);
    server->on("/0wifi", std::bind(&CustomWiFiManager::handleWifi, this, std::placeholders::_1, false)).setFilter(ON_AP_FILTER);
    server->on("/wifisave", std::bind(&CustomWiFiManager::handleWifiSave, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    server->on("/i", std::bind(&CustomWiFiManager::handleInfo, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    server->on("/r", std::bind(&CustomWiFiManager::handleReset, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    server->on("/fwlink", std::bind(&CustomWiFiManager::handleRoot, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    server->onNotFound(std::bind(&CustomWiFiManager::handleNotFound, this, std::placeholders::_1));

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

void CustomWiFiManager::connectTask() {
    if (retry == 0) {
        // attempt to connect;
        WiFi.mode(WIFI_STA);
        connectWifi("", "");
        retry = 1;
    } else if (WiFi.status() == WL_CONNECTED) {
        retry = 1; // not 0, would reconnect!
    } else {
        if (++retry == WIFI_MANAGER_MAX_RETRIES) {
            retry = 0; // re-initialize
        }
    }
}


String CustomWiFiManager::networkListAsString() {
    String result;
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
            result += item;

        }
    }
    return result;
}

void CustomWiFiManager::scan() {
    delay(100);
    wifi_ssid_count_t n = WiFi.scanNetworks();

    // error
    if (n < 0) {
        return;
    }

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

void CustomWiFiManager::setInfo() {
    infoPage = infoAsString();
    wifiStatus = WiFi.status();
}

void CustomWiFiManager::enableConfigPortal(char const *apName, char const *apPassword) {
    //setup AP
    WiFi.mode(WIFI_AP_STA);

    _apName = apName;
    _apPassword = apPassword;

    connect = false;
    setupConfigPortal();
}

void CustomWiFiManager::disableWifi() {
#if defined(ESP8266)
    // we might still be connecting, so that has to stop for scanning
    ETS_UART_INTR_DISABLE ();
    wifi_station_disconnect();
    ETS_UART_INTR_ENABLE ();
#else
    WiFi.disconnect (false);
#endif
}

void CustomWiFiManager::scanNetworks() {
    disableWifi();
    scan();
    WiFi.begin(); // try to reconnect to AP
}

// do a single iteration through the configuration loop
bool CustomWiFiManager::configTask() {

    // attempts to reconnect were successful
    if (WiFi.status() == WL_CONNECTED) {
        //connected
        WiFi.mode(WIFI_STA);
        //notify that configuration has changed and any optional parameters should be saved
        if (_savecallback != nullptr) {
            //todo: check if any custom parameters actually exist, and check if they really changed maybe
            _savecallback();
        }
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
        }
    }
    return WiFi.status() == WL_CONNECTED;
}


int CustomWiFiManager::connectWifi(const String &ssid, const String &pass) {

    //check if we have ssid and pass and force those, if not, try with last saved values
    if (ssid != "") {
        disableWifi();
        WiFi.begin(ssid.c_str(), pass.c_str());
    } else {
        if (WiFi.SSID().length() > 0) {
            disableWifi();

            WiFi.begin();
        } else {
            WiFi.begin();
        }
    }

    wl_status_t connectionStatus = WiFi.status();
    bool connection_ok = connectionStatus == WL_CONNECTED;
    if (connection_ok) {
        setInfo();
    }
    return connection_ok;
}

void CustomWiFiManager::resetSettings() {
    WiFi.disconnect(true);
}

void CustomWiFiManager::setMinimumSignalQuality(int quality) {
    _minimumQuality = quality;
}

/** Handle root or redirect to captive portal */
void CustomWiFiManager::handleRoot(AsyncWebServerRequest *request) {
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
    page += FPSTR(HTTP_PORTAL_OPTIONS);
    page += _customOptionsElement;
    page += FPSTR(HTTP_END);

    request->send(200, "text/html", page);

}

/** Wifi config page handler */
void CustomWiFiManager::handleWifi(AsyncWebServerRequest *request, boolean scan) {
    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Config ESP");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _customHeadElement;
    page += FPSTR(HTTP_HEAD_END);

    if (scan) {
        if (wifiSSIDCount == 0) {
            page += F("No networks found. Refresh to scan again.");
        } else {
            //display networks in page
            String result = networkListAsString();

            page += result;
            page += "<br/>";
        }

    }

    page += FPSTR(HTTP_FORM_START);
    char parLength[3];
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
void CustomWiFiManager::handleWifiSave(AsyncWebServerRequest *request) {

    //SAVE/connect here
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
String CustomWiFiManager::infoAsString() {
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

void CustomWiFiManager::handleInfo(AsyncWebServerRequest *request) {
    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Info");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _customHeadElement;
    if (connect == true) {
        page += F("<meta http-equiv=\"refresh\" content=\"5; url=/i\">");
    }
    page += FPSTR(HTTP_HEAD_END);
    page += F("<dl>");

    if (connect == true) {
        page += F("<dt>Trying to connect</dt><dd>");
        page += wifiStatus;
        page += F("</dd>");
    }

    page += infoPage;
    page += FPSTR(HTTP_END);

    request->send(200, "text/html", page);
}

/** Handle the reset page */
void CustomWiFiManager::handleReset(AsyncWebServerRequest *request) {
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

void CustomWiFiManager::handleNotFound(AsyncWebServerRequest *request) {
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

    for (size_t i = 0; i < request->args(); i++) {
        message += " " + request->argName(i) + ": " + request->arg(i) + "\n";
    }
    AsyncWebServerResponse *response = request->beginResponse(404, "text/plain", message);
    response->addHeader("Cache-Control", "no-cache, no-store, must-revalidate");
    response->addHeader("Pragma", "no-cache");
    response->addHeader("Expires", "-1");
    request->send(response);
}


/** Redirect to captive portal if we got a request for another domain. Return true in that case so the page handler do not try to handle the request again. */
boolean CustomWiFiManager::captivePortal(AsyncWebServerRequest *request) {
    if (!isIp(request->host())) {
        AsyncWebServerResponse *response = request->beginResponse(302, "text/plain", "");
        response->addHeader("Location", String("http://") + toStringIp(request->client()->localIP()));
        request->send(response);
        return true;
    }
    return false;
}

//start up save config callback
void CustomWiFiManager::setSaveConfigCallback(void (*func)()) {
    _savecallback = func;
}

//sets a custom element to add to head, like a new style tag
void CustomWiFiManager::setCustomHeadElement(const char *element) {
    _customHeadElement = element;
}

//sets a custom element to add to options page
void CustomWiFiManager::setCustomOptionsElement(const char *element) {
    _customOptionsElement = element;
}

int CustomWiFiManager::getRSSIasQuality(const int RSSI) {
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
boolean CustomWiFiManager::isIp(const String &str) {
    for (unsigned int i = 0; i < str.length(); i++) {
        int c = str.charAt(i);
        if (c != '.' && (c < '0' || c > '9')) {
            return false;
        }
    }
    return true;
}

/** IP to String? */
String CustomWiFiManager::toStringIp(const IPAddress &ip) {
    String res = "";
    for (int i = 0; i < 3; i++) {
        res += String((ip >> (8 * i)) & 0xFF) + ".";
    }
    res += String(((ip >> 8 * 3)) & 0xFF);
    return res;
}
