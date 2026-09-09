#include "display.hpp"

#include "Formatting.hpp"
#include "Server.hpp"
#include "System.hpp"
#include "Wifi.hpp"

#include <Arduino.h>


// pin definitions
const uint8_t PIN_BTN_RESET = 9;

const uint8_t EPD_SDI = 4;
const uint8_t EPD_SCK = 5;
const uint8_t SPI_MISO = 8;  // not used

const uint8_t EPD_SS = 6;
const uint8_t EPD_DC = 7;
const uint8_t EPD_RST = 2;
const uint8_t EPD_BUSY = 3;


// devices
EPD epd(EPD_SS, EPD_DC, EPD_RST, EPD_BUSY);


void setup()
{
  Serial.begin(115200);

  // Display
  SPI.begin(EPD_SCK, SPI_MISO, EPD_SDI, EPD_SS);
  epd.init();
  epd.hibernate();

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


constexpr size_t BUFSIZE = EPD::WIDTH * EPD::HEIGHT / 8;
uint8_t px_black[BUFSIZE];
uint8_t px_color[BUFSIZE];


void showImage(
        const uint8_t* pixel_data,
        size_t width,
        size_t height)
{
    for (size_t y = 0; y < EPD::HEIGHT; ++y)
    {
        for (size_t x8 = 0; x8 < EPD::WIDTH / 8; ++x8)
        {
            uint8_t red   = 0;
            uint8_t green = 0;
            uint8_t blue  = 0;
            uint8_t alpha = 0;

            for (int s = 0; s < 8; ++s)
            {
                const size_t x = x8 * 8 + s;
                const size_t j = x * EPD::HEIGHT + y;

                if (x < width and y < height) {
                    red   |= ((pixel_data[j] & 1) >> 0) << (7 - s);
                    green |= ((pixel_data[j] & 2) >> 1) << (7 - s);
                    blue  |= ((pixel_data[j] & 4) >> 2) << (7 - s);
                    alpha |= ((pixel_data[j] & 8) >> 3) << (7 - s);
                }
            }

            const uint8_t white = (red & green & blue) | ~alpha;
            const uint8_t color = (red | green | blue) & ~white;
            const uint8_t black = ~white & ~color;

            const size_t i = (EPD::HEIGHT - y - 1) * EPD::WIDTH / 8 + x8;

            px_black[i] = ~black;
            px_color[i] = ~color;
        }
    }

    epd.drawImage(px_black, px_color, 0, 0, EPD::WIDTH, EPD::HEIGHT);
}
