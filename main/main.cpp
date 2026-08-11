#include "Formatting.hpp"
#include "Server.hpp"
#include "System.hpp"
#include "Wifi.hpp"

#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <GxEPD2_3C.h>


// pin definitions
const uint8_t PIN_BTN_RESET = 13;

const uint8_t EPD_SDI = 23;
const uint8_t EPD_SCK = 18;
const uint8_t SPI_MISO = 19;  // not used

const uint8_t EPD_SS = 5;
const uint8_t EPD_DC = 26;
const uint8_t EPD_RST = 27;
const uint8_t EPD_BUSY = 14;


// devices
typedef GxEPD2_290_C90c EPD;

GxEPD2_3C<EPD, EPD::HEIGHT> display(EPD(EPD_SS, EPD_DC, EPD_RST, EPD_BUSY));


void setup()
{
  Serial.begin(115200);

#if 0
  SPI.setSCK(EPD_SCK);
  SPI.setTX(EPD_SDI);
#else
  SPI.begin(EPD_SCK, SPI_MISO, EPD_SDI, EPD_SS);
#endif
  display.init();
  display.hibernate();

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
    delay(10);
}


void showText(String text)
{
    Serial << "Text: " << text << endl;

  display.setRotation(3);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextSize(2, 2);
  display.setTextColor(GxEPD_BLACK);

  int16_t tbx, tby;
  uint16_t tbw, tbh;
  display.getTextBounds(text, 0, 0, &tbx, &tby, &tbw, &tbh);
  uint16_t x = ((display.width() - tbw) / 2) - tbx;
  uint16_t y = ((display.height() - tbh) / 2) - tby;

  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_RED);
    const int margin = 8;
    display.fillRoundRect(x - margin, y - tbh - margin, tbw + 2*margin, tbh + 2*margin, 4, GxEPD_WHITE);
    display.setCursor(x, y);
    display.print(text);
  } while (display.nextPage());

  display.hibernate();
}
