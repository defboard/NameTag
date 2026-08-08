#include "Server.hpp"
#include "System.hpp"
#include "Wifi.hpp"

#include <Arduino.h>


// pin definitions
const uint8_t PIN_BTN_RESET = 13;


void setup()
{
  Serial.begin(115200);

  // Load saved settings
  if (digitalRead(PIN_BTN_RESET) == HIGH)
  {
    Preferences prefs;
    bool open = prefs.begin(PREFS_NAMESPACE, /* readOnly */ true);
    if (open) {
      loadWifiSettings(prefs);
    }
  }

  // Network
  initWifi();
  initWebServer();
}


void loop()
{
}
