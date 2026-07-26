#ifndef CYCOM_SRC_PRESENTATION_DISPLAY_COLOR_H_
#define CYCOM_SRC_PRESENTATION_DISPLAY_COLOR_H_

#include <cstdint>

namespace ui {

struct Color565 {
    uint16_t value;  // RGB565
    static Color565 RGB(uint8_t r, uint8_t g, uint8_t b);
    static Color565 Black();
    static Color565 White();
    static Color565 Gray();
};

}  // namespace ui

#endif  // CYCOM_SRC_PRESENTATION_DISPLAY_COLOR_H_
