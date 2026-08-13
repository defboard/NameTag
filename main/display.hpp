#pragma once
#include <GxEPD2_3C.h>

typedef GxEPD2_290_C90c EPD;


void showImage(
        const uint8_t* pixel_data,
        size_t width=EPD::WIDTH,
        size_t height=EPD::HEIGHT,
        uint8_t threshold=127);
