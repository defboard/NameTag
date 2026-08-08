#include "Server.hpp"

#include "Formatting.hpp"
#include "HTTPUpdateServer.h"
#include "Json.hpp"
#include "System.hpp"
#include "Wifi.hpp"

#include <Preferences.h>
#include <WebServer.h>

#include <freertos/task.h>

#include <optional>

// Globals
WebServer server(80);
HTTPUpdateServer updateServer;

extern const uint8_t static_index_html_start[]  asm("_binary_index_html_gz_start");
extern const uint8_t static_index_html_end[]    asm("_binary_index_html_gz_end");

extern const uint8_t static_hyperapp_js_start[] asm("_binary_hyperapp_js_gz_start");
extern const uint8_t static_hyperapp_js_end[]   asm("_binary_hyperapp_js_gz_end");


// Forward declarations
void onHttpRoot();
void onHttpHyperappJs();
void onHttpFileEventsLog();
void onHttpApiStatus();
void onHttpApiServerReboot();
void onHttpApiPrefsPost();
void sendFile(int code, const char* content_type, const uint8_t* start, const uint8_t* end);


namespace {
  std::optional<WiFiMode_t> strToWifiMode(const String& wifi_mode)
  {
    if (wifi_mode == "OFF") { return WIFI_OFF; }
    if (wifi_mode == "STA") { return WIFI_STA; }
    if (wifi_mode == "AP") { return WIFI_AP; }
    return std::nullopt;
  }

  const char* dumpWifiMode(WiFiMode_t wifi_mode)
  {
    if (wifi_mode == WIFI_OFF) { return "OFF"; }
    if (wifi_mode == WIFI_STA) { return "STA"; }
    if (wifi_mode == WIFI_AP) { return "AP"; }
    return "OFF";
  }
}


// Implementation

void initWebServer()
{
  server.on("/", onHttpRoot);
  server.on("/hyperapp.js", onHttpHyperappJs);
  server.on("/file/events.log", onHttpFileEventsLog);
  server.on("/api/status", onHttpApiStatus);
  server.on("/api/server/reboot", onHttpApiServerReboot);
  server.on("/prefs", HTTPMethod::HTTP_POST, onHttpApiPrefsPost);

  updateServer.setup(&server, "/update");
  server.begin();

  xTaskCreatePinnedToCore(handleServer, "server", 4096, NULL, 1, NULL, 0);
}

void handleServer(void* args)
{
  while (true) {
    server.handleClient();
  }
}

void sendFile(int code, const char* content_type, const uint8_t* start, const uint8_t* end)
{
  const int size = end - start;
  server.setContentLength(size);
  server.send(code, content_type);
  server.sendContent((const char*) start, size);
}

void onHttpRoot()
{
  server.sendHeader("Content-Encoding", "gzip");
  sendFile(200, "text/html", static_index_html_start, static_index_html_end);
}

void onHttpHyperappJs()
{
  server.sendHeader("Content-Encoding", "gzip");
  sendFile(200, "text/javascript", static_hyperapp_js_start, static_hyperapp_js_end);
}

void onHttpFileEventsLog()
{
  server.send(200, "text/plain", (String&) eventLog);
}

void onHttpApiStatus()
{
  StreamString response;
  JsonWriter json(response);

  json.put_object();
  json.put_string("wifiMode", dumpWifiMode(WIFI_MODE));
  json.put_string("wifiSsid", WIFI_SSID);
  json.put_string("wifiHostname", WIFI_HOSTNAME);
  json.end_object();

  server.send(200, "application/json", (String&) response);
}

void onHttpApiServerReboot()
{
  server.send(200, "application/json", "{}");
  delay(100);
  esp_restart();
}

void onHttpApiPrefsPost()
{
  Preferences prefs;
  prefs.begin(PREFS_NAMESPACE, /* readOnly */ false);

  // WiFi settings
  String wifi_mode = server.arg("wifiMode");
  String wifi_ssid = server.arg("wifiSsid");
  String wifi_password = server.arg("wifiPassword");
  String wifi_hostname = server.arg("wifiHostname");

  std::optional<WiFiMode_t> mode = strToWifiMode(wifi_mode);

  bool restart_wifi = false;

  if (mode == WIFI_OFF and WIFI_MODE != WIFI_OFF) {
    disableWifi(prefs);
    restart_wifi = true;
  }
  if (mode and mode != WIFI_OFF and wifi_ssid.length() > 0 and wifi_password.length() > 0) {
    if (mode != WIFI_MODE or wifi_ssid != WIFI_SSID or wifi_password != WIFI_PASSWORD) {
      setWifiNetwork(prefs, *mode, wifi_ssid, wifi_password);
      restart_wifi = true;
    }
  }
  if (mode and mode != WIFI_OFF and wifi_hostname.length() > 0) {
    if (wifi_hostname != WIFI_HOSTNAME) {
      setWifiHostname(prefs, wifi_hostname);
      restart_wifi = true;
    }
  }

  // Send status update before disconnecting WiFi
  onHttpApiStatus();

  if (restart_wifi) {
    stopWifi();
    initWifi();
  }
}
