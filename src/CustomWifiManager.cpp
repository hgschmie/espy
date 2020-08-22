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

/*
 * Custom parameters
 */

CustomWiFiManagerParameter::CustomWiFiManagerParameter(const char *id, const char *placeholder, const char *defaultValue, int length, const char *custom)
        : _id(id), _placeholder(placeholder), _length(length), _customHTML(custom) {

    _value = new char[_length + 1];
    memset(_value, '\0', _length + 1);

    if (defaultValue != nullptr) {
        strncpy(_value, defaultValue, length);
    }
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

/*
 * WifiManager code
 */

CustomWiFiManager::CustomWiFiManager(AsyncWebServer *server)
        : _server(server), _config_portal_last_wifi_scan_networks(nullptr) {
}

/*
 * Task for the regular operation. Drives connection to the Wifi.
 */
void CustomWiFiManager::connectTask() {
    if (connectionRetries == 0) {
        // attempt to connect;
        WiFi.mode(WIFI_STA);
        // use stored credentials
        connectWifi();
        connectionRetries = 1;
    } else if (WiFi.status() == WL_CONNECTED) {
        connectionRetries = 1; // not 0, would reconnect!
    } else {
        if (++connectionRetries >= (WIFI_MANAGER_MAX_RETRY_TIME_MS / WIFI_MANAGER_CONNECTION_TASK_TIME_MS)) {
            connectionRetries = 0; // re-initialize
        }
    }
}

// do a single iteration through the configuration loop
// driven out of the config menu, return true if the portal has
// successfully reconfigured the WiFi.
bool CustomWiFiManager::configPortalMenu() {

    // <0: don't bother, return not connected right away.
    if (_config_portal_connect_retries < 0) {
        return false;
    } else if (_config_portal_connect_retries == 0) { // 0: Initialize WiFi, increment retries
        // initialize configuration
        connectWifi(&_config_portal_ssid, &_config_portal_password);
        _config_portal_connect_retries = 1;

    } else if (WiFi.status() == WL_CONNECTED) { //connected, switch back to station mode
        WiFi.mode(WIFI_STA);

        //notify that configuration has changed and any optional parameters should be saved
        if (_config_portal_save_settings_callback != nullptr) {
            //todo: check if any custom parameters actually exist, and check if they really changed maybe
            _config_portal_save_settings_callback();
        }
        return true;

        // >0: run retry to wait for connection, reset at timeout
    } else if (_config_portal_connect_retries > 0) {
        // retry at the menu loop interval period; reset and restart the wifi after that.
        if (++_config_portal_connect_retries >= (WIFI_MANAGER_MAX_CONFIG_RETRY_TIME_MS / WIFI_MANAGER_CONFIG_MENU_TASK_TIME_MS)) {
            _config_portal_connect_retries = 0;
        }
    }

    return false; // not connected.
}

/*
 * Start the configuration portal mode.
 */
void CustomWiFiManager::enableConfigPortal(char const *apName, char const *apPassword) {
    //setup AP
    WiFi.mode(WIFI_AP_STA);

    _config_portal_ap_name = apName;
    _config_portal_ap_password = apPassword;

    // do not try to connect, menu loop will return false (not connected)
    _config_portal_connect_retries = -1;

    if (_config_portal_ap_password != nullptr) {
        if (strlen(_config_portal_ap_password) < 8 || strlen(_config_portal_ap_password) > 63) {
            // fail passphrase to short or long!
            _config_portal_ap_password = nullptr;
        }
    }

    if (_config_portal_ap_password != nullptr) {
        WiFi.softAP(_config_portal_ap_name, _config_portal_ap_password);//password option
    } else {
        WiFi.softAP(_config_portal_ap_name);
    }

    delay(500); // Without delay I've seen the IP address blank

    updateInfo();

    /* Setup web pages: root, wifi config pages, SO captive portal detectors and not found. */
    _server->on("/", std::bind(&CustomWiFiManager::handleRoot, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    _server->on("/wifi", std::bind(&CustomWiFiManager::handleWifi, this, std::placeholders::_1, true)).setFilter(ON_AP_FILTER);
    _server->on("/0wifi", std::bind(&CustomWiFiManager::handleWifi, this, std::placeholders::_1, false)).setFilter(ON_AP_FILTER);
    _server->on("/wifisave", std::bind(&CustomWiFiManager::handleWifiSave, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    _server->on("/i", std::bind(&CustomWiFiManager::handleInfo, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    _server->on("/r", std::bind(&CustomWiFiManager::handleReset, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    //Microsoft captive portal. Maybe not needed. Might be handled by notFound handler.
    _server->on("/fwlink", std::bind(&CustomWiFiManager::handleRoot, this, std::placeholders::_1)).setFilter(ON_AP_FILTER);
    _server->onNotFound(std::bind(&CustomWiFiManager::handleNotFound, this, std::placeholders::_1));

    _server->begin(); // Web server start
}

void CustomWiFiManager::scanNetworkTask() {
    disableWifi();
    scan();
    WiFi.begin(); // try to reconnect to AP
}

void CustomWiFiManager::resetSettings() {
    WiFi.disconnect(true);
    // restart connection attempts
    connectionRetries = 0;
    // disable config portal connection attempts
    _config_portal_connect_retries = -1;
}


void CustomWiFiManager::setMinimumSignalQuality(int quality) {
    _config_minimum_quality = quality;
}

void CustomWiFiManager::setSaveConfigCallback(void (*func)()) {
    _config_portal_save_settings_callback = func;
}

void CustomWiFiManager::addParameter(CustomWiFiManagerParameter *p) {
    _custom_config_parameters[_custom_current_param_index] = p;
    _custom_current_param_index++;
}

//sets a custom element to add to head, like a new style tag
void CustomWiFiManager::setCustomHeadElement(const char *element) {
    _config_custom_html_head = element;
}

//sets a custom element to add to options page
void CustomWiFiManager::setCustomOptionsElement(const char *element) {
    _config_custom_html_options = element;
}

// ---------------------------------------- END PUBLIC

#if !defined(ESP8266)
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

// ---------------------------------------- PRIVATE


// Disables Wifi if currently connected and restarts
// with the given credentials (or stored credentials if ssid is the empty string
bool CustomWiFiManager::connectWifi(const String *ssid, const String *pass) {

    // already connected, disconnect
    if (WiFi.SSID().length() > 0) {
        disableWifi();
    }

    //check if we have ssid and pass and force those, if not, try with last saved values
    if (ssid != nullptr) {
        if (pass != nullptr) {
            WiFi.begin(ssid->c_str(), pass->c_str()); // don't pass nullptr as pass or ssid
        } else {
            WiFi.begin(ssid->c_str());
        }
    } else {
        WiFi.begin();
    }

    wl_status_t connectionStatus = WiFi.status();
    bool connection_ok = connectionStatus == WL_CONNECTED;
    if (connection_ok) {
        updateInfo();
    }
    return connection_ok;
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

void CustomWiFiManager::scan() {
    wifi_ssid_count_t n = WiFi.scanNetworks();

    if (n < 0) {
        return; // error returned. Ignore, try again later.
    }

    // this whole code piece sucks. It works well enough and is almost never run. No point in cleaning.

    /* WE SHOULD MOVE THIS IN PLACE ATOMICALLY */
    if (_config_portal_last_wifi_scan_networks) {
        delete[] _config_portal_last_wifi_scan_networks;
    }

    _config_portal_last_wifi_scan_networks = new WiFiResult[n];
    _config_portal_last_wifi_scan_count = n;

    for (wifi_ssid_count_t i = 0; i < n; i++) {
        _config_portal_last_wifi_scan_networks[i].duplicate = false;

// TODO - check res to see whether the network is valid. Be smarter in adding those to the list.
#if defined(ESP8266)
        WiFi.getNetworkInfo(i, _config_portal_last_wifi_scan_networks[i].SSID, _config_portal_last_wifi_scan_networks[i].encryptionType,
                            _config_portal_last_wifi_scan_networks[i].RSSI, _config_portal_last_wifi_scan_networks[i].BSSID,
                            _config_portal_last_wifi_scan_networks[i].channel, _config_portal_last_wifi_scan_networks[i].isHidden);
#else
        WiFi.getNetworkInfo(i, wifiSSIDs[i].SSID, wifiSSIDs[i].encryptionType, wifiSSIDs[i].RSSI, wifiSSIDs[i].BSSID, wifiSSIDs[i].channel);
#endif
    }


    // RSSI SORT

    // old sort
    for (int i = 0; i < n; i++) {
        for (int j = i + 1; j < n; j++) {
            if (_config_portal_last_wifi_scan_networks[j].RSSI > _config_portal_last_wifi_scan_networks[i].RSSI) {
                std::swap(_config_portal_last_wifi_scan_networks[i], _config_portal_last_wifi_scan_networks[j]);
            }
        }
    }


    // remove duplicates ( must be RSSI sorted )
    String cssid;
    for (int i = 0; i < n; i++) {
        if (_config_portal_last_wifi_scan_networks[i].duplicate == true) continue;
        cssid = _config_portal_last_wifi_scan_networks[i].SSID;
        for (int j = i + 1; j < n; j++) {
            if (cssid == _config_portal_last_wifi_scan_networks[j].SSID) {
                _config_portal_last_wifi_scan_networks[j].duplicate = true; // set dup aps to NULL
            }
        }
    }
}

void CustomWiFiManager::updateInfo() {
    _cache_infoPage = infoAsHtml();
    _cache_wifiStatus = WiFi.status();
}

// ---------------------------------------- WEBSERVER STUFF

/** Handle root or redirect to captive portal */
void CustomWiFiManager::handleRoot(AsyncWebServerRequest *request) {
    if (captivePortal(request)) { // If captive portal redirect instead of displaying the page.
        return;
    }

    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Options");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _config_custom_html_head;
    page += FPSTR(HTTP_HEAD_END);
    page += "<h1>";
    page += _config_portal_ap_name;
    page += "</h1>";
    page += FPSTR(HTTP_PORTAL_OPTIONS);
    page += _config_custom_html_options;
    page += FPSTR(HTTP_END);

    request->send(200, "text/html", page);

}

/** Wifi config page handler */
void CustomWiFiManager::handleWifi(AsyncWebServerRequest *request, boolean scan) {
    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Config ESP");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _config_custom_html_head;
    page += FPSTR(HTTP_HEAD_END);

    if (scan) {
        if (_config_portal_last_wifi_scan_count == 0) {
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
    for (int i = 0; i < _custom_current_param_index; i++) {
        if (_custom_config_parameters[i] == nullptr) {
            break;
        }

        String pitem = FPSTR(HTTP_FORM_PARAM);
        if (_custom_config_parameters[i]->getID() != nullptr) {
            pitem.replace("{i}", _custom_config_parameters[i]->getID());
            pitem.replace("{n}", _custom_config_parameters[i]->getID());
            pitem.replace("{p}", _custom_config_parameters[i]->getPlaceholder());
            snprintf(parLength, 2, "%d", _custom_config_parameters[i]->getValueLength());
            pitem.replace("{l}", parLength);
            pitem.replace("{v}", _custom_config_parameters[i]->getValue());
            pitem.replace("{c}", _custom_config_parameters[i]->getCustomHTML());
        } else {
            pitem = _custom_config_parameters[i]->getCustomHTML();
        }

        page += pitem;
    }
    if (_custom_config_parameters[0] != nullptr) {
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
    _config_portal_ssid = request->arg("s").c_str();
    _config_portal_password = request->arg("p").c_str();

    //parameters
    for (int i = 0; i < _custom_current_param_index; i++) {
        if (_custom_config_parameters[i] == nullptr) {
            break;
        }
        //read parameter
        String value = request->arg(_custom_config_parameters[i]->getID()).c_str();
        //store it in array
        value.toCharArray(_custom_config_parameters[i]->_value, _custom_config_parameters[i]->_length);
    }

    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Credentials Saved");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _config_custom_html_head;
    page += F("<meta http-equiv=\"refresh\" content=\"5; url=/i\">");
    page += FPSTR(HTTP_HEAD_END);
    page += FPSTR(HTTP_SAVED);
    page += FPSTR(HTTP_END);

    request->send(200, "text/html", page);

    // config stored, start connecting in the menu loop
    _config_portal_connect_retries = 0;
}

void CustomWiFiManager::handleInfo(AsyncWebServerRequest *request) {
    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Info");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _config_custom_html_head;
    // add wifi status if the chip is trying to connect
    if (_config_portal_connect_retries >= 0) {
        page += F("<meta http-equiv=\"refresh\" content=\"5; url=/i\">");
    }
    page += FPSTR(HTTP_HEAD_END);
    page += F("<dl>");

    if (_config_portal_connect_retries >= 0) {
        page += F("<dt>Trying to connect</dt><dd>");
        page += _cache_wifiStatus;
        page += F("</dd>");
    }

    page += _cache_infoPage;
    page += FPSTR(HTTP_END);

    request->send(200, "text/html", page);
}

/** Handle the reset page */
void CustomWiFiManager::handleReset(AsyncWebServerRequest *request) {
    String page = FPSTR(WFM_HTTP_HEAD);
    page.replace("{v}", "Info");
    page += FPSTR(HTTP_SCRIPT);
    page += FPSTR(HTTP_STYLE);
    page += _config_custom_html_head;
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

// ---------------------------------------- HELPERS


/** Handle the info page */
String CustomWiFiManager::infoAsHtml() {
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
// ---------------------------------------- HELPERS


String CustomWiFiManager::networkListAsString() {
    String result;
    //display networks in page
    for (int i = 0; i < _config_portal_last_wifi_scan_count; i++) {
        if (_config_portal_last_wifi_scan_networks[i].duplicate == true) continue; // skip dups
        int quality = getRSSIasQuality(_config_portal_last_wifi_scan_networks[i].RSSI);

        if (_config_minimum_quality == -1 || _config_minimum_quality < quality) {
            String item = FPSTR(HTTP_ITEM);
            item.replace("{v}", _config_portal_last_wifi_scan_networks[i].SSID);
            item.replace("{r}", String(quality));
#if defined(ESP8266)
            if (_config_portal_last_wifi_scan_networks[i].encryptionType != ENC_TYPE_NONE) {
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
        char c = str.charAt(i);
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
