#include "presentation/display/color.h"

namespace ui {

Color565 Color565::RGB(uint8_t r, uint8_t g, uint8_t b) {
    uint16_t v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
    return {v};
}

Color565 Color565::Black() {
    return {0x0000};
}

Color565 Color565::White() {
    return {0xFFFF};
}

Color565 Color565::Gray() {
    return {0x7BEF};
}

}  // namespace ui
