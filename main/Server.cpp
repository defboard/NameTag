#include "Server.hpp"
#include "display.hpp"

#include "Formatting.hpp"
#include "HTTPUpdateServer.h"
#include "Json.hpp"
#include "System.hpp"
#include "Wifi.hpp"

#include <Preferences.h>
#include <WebServer.h>

#include <freertos/task.h>

#include <cstring>
#include <optional>


// Globals
WebServer server(80);
HTTPUpdateServer updateServer;

extern const uint8_t static_index_html_start[]  asm("_binary_index_html_gz_start");
extern const uint8_t static_index_html_end[]    asm("_binary_index_html_gz_end");

extern const uint8_t static_hyperapp_js_start[] asm("_binary_hyperapp_js_gz_start");
extern const uint8_t static_hyperapp_js_end[]   asm("_binary_hyperapp_js_gz_end");

constexpr size_t maxUploadFileSize = EPD::WIDTH * EPD::HEIGHT * 4;
uint8_t uploadFileBuffer[maxUploadFileSize / 4];
uint8_t colorThreshold = 127;


// Forward declarations
void onHttpRoot();
void onHttpHyperappJs();
void onHttpFileEventsLog();
void onHttpApiStatus();
void onHttpApiImage();
void onHttpApiImageUpload();
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
  server.on("/api/image", HTTPMethod::HTTP_POST, onHttpApiImage, onHttpApiImageUpload);
  server.on("/prefs", HTTPMethod::HTTP_POST, onHttpApiPrefsPost);

  updateServer.setup(&server, "/update");
  server.begin();

  xTaskCreatePinnedToCore(handleServer, "server", 4096, NULL, 1, NULL, 1);
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
  // WIDTH and HEIGHT are switched because EPD assumes the display is used
  // in portrait mode, while I prefer landscape:
  json.put_plain("displayWidth", (int) EPD::HEIGHT);
  json.put_plain("displayHeight", (int) EPD::WIDTH);
  json.end_object();

  server.send(200, "application/json", (String&) response);
}

void onHttpApiImage()
{
    HTTPUpload& upload = server.upload();

    bool success = upload.totalSize == maxUploadFileSize;

    StreamString message;
    if (success) {
        message << "Upload successful.";
    }
    else {
        message << "Upload failed due to unexpected file size."
            << "\nFile size must be " << maxUploadFileSize << " bytes."
            << "\nReceived " << upload.totalSize << " bytes.";
    }

    StreamString response;
    JsonWriter json(response);

    json.put_object();
    json.put_bool("success", success);
    json.put_string("message", message);
    json.end_object();
    server.send(200, "application/json", (String&) response);

    if (success) {
        showImage(uploadFileBuffer, EPD::WIDTH, EPD::HEIGHT);
    }
}

void onHttpApiImageUpload()
{
    HTTPUpload& upload = server.upload();
    size_t pos = 0;
    size_t end = 0;

    switch (upload.status) {
        case UPLOAD_FILE_START:
            std::memset(uploadFileBuffer, 0, sizeof(uploadFileBuffer));
            break;

        case UPLOAD_FILE_WRITE:
            pos = std::min(maxUploadFileSize, upload.totalSize);
            end = std::min(maxUploadFileSize, upload.totalSize + upload.currentSize);
            for (size_t i = 0; pos < end; ++pos, ++i) {
                bool bit = ((uint8_t) upload.buf[i]) > colorThreshold;
                uploadFileBuffer[pos / 4] |= bit << (pos % 4);
            }
            break;

        case UPLOAD_FILE_END:
            break;

        case UPLOAD_FILE_ABORTED:
            break;
    }
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
