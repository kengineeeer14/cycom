#include "display/st7796.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>
#include <unistd.h>  // usleep

namespace st7796 {

Display::Display()
    : dc_("gpiochip0", kDC,  true, 1)
    , rst_("gpiochip0", kRST, true, 1)
    , bl_("gpiochip0",  kBL,  true, 1)
    , spi_("/dev/spidev0.0", 40000000, 0, 8)  // 必要なら 12000000 などに下げてテスト
{
    reset();
    init();
    bl_.set(1);  // バックライトON
}

void Display::clear(uint16_t rgb565) {
    setAddressWindow(0, 0, kWidth - 1, kHeight - 1);
    dataMode(true);
    std::vector<uint8_t> line(kWidth * 2);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            line[2 * x + 0] = static_cast<uint8_t>((rgb565 >> 8) & 0xFF);
            line[2 * x + 1] = static_cast<uint8_t>( rgb565        & 0xFF);
        }
        sendChunked(line.data(), line.size());
    }
}

void Display::drawFilledRect(int x0, int y0, int x1, int y1, uint16_t rgb565) {
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    x0 = std::max(0, x0); y0 = std::max(0, y0);
    x1 = std::min(kWidth  - 1, x1);
    y1 = std::min(kHeight - 1, y1);

    if (x0 > x1 || y0 > y1) return;  // クリップ後に完全画面外

    setAddressWindow(x0, y0, x1, y1);
    dataMode(true);

    const int w = (x1 - x0 + 1);
    std::vector<uint8_t> line(w * 2);
    for (int i = 0; i < w; ++i) {
        line[2 * i + 0] = static_cast<uint8_t>((rgb565 >> 8) & 0xFF);
        line[2 * i + 1] = static_cast<uint8_t>( rgb565        & 0xFF);
    }
    for (int y = y0; y <= y1; ++y) {
        sendChunked(line.data(), line.size());
    }
}

void Display::blitRGB565(const uint8_t* buf, size_t len) {
    const size_t expected = static_cast<size_t>(kWidth) * kHeight * 2;
    if (len != expected) throw std::runtime_error("blitRGB565: size mismatch");
    setAddressWindow(0, 0, kWidth - 1, kHeight - 1);
    dataMode(true);
    sendChunked(buf, len);
}

void Display::dataMode(bool data) {
    dc_.set(data ? 1 : 0);
}

void Display::cmd(uint8_t c) {
    dataMode(false);
    spi_.writeBytes(&c, 1);
}

void Display::dat(uint8_t d) {
    dataMode(true);
    spi_.writeBytes(&d, 1);
}

void Display::reset() {
    rst_.set(1); usleep(10000);
    rst_.set(0); usleep(10000);
    rst_.set(1); usleep(10000);
}

void Display::setAddressWindow(int xs, int ys, int xe, int ye) {
    // CASET (0x2A): X
    cmd(0x2A);
    dat(static_cast<uint8_t>((xs >> 8) & 0xFF)); dat(static_cast<uint8_t>(xs & 0xFF));
    dat(static_cast<uint8_t>((xe >> 8) & 0xFF)); dat(static_cast<uint8_t>(xe & 0xFF));
    // RASET (0x2B): Y
    cmd(0x2B);
    dat(static_cast<uint8_t>((ys >> 8) & 0xFF)); dat(static_cast<uint8_t>(ys & 0xFF));
    dat(static_cast<uint8_t>((ye >> 8) & 0xFF)); dat(static_cast<uint8_t>(ye & 0xFF));
    // RAMWR
    cmd(0x2C);
}

void Display::sendChunked(const uint8_t* data, size_t len) {
    const size_t CHUNK = 4096;
    size_t off = 0;
    while (off < len) {
        const size_t n = std::min(CHUNK, len - off);
        spi_.writeBytes(data + off, n);
        off += n;
    }
}

void Display::init() {
    // Sleep Out
    cmd(0x11); usleep(120000);

    // MADCTL: 縦（0x08）。必要なら 0x68 / 0xA8 なども試す
    cmd(0x36); dat(0x08);

    // Pixel format: 16bpp
    cmd(0x3A); dat(0x55); // 0x55=16bit/px

    // 以降、Python版に寄せた初期値
    cmd(0xF0); dat(0xC3);
    cmd(0xF0); dat(0x96);
    cmd(0xB4); dat(0x01);
    cmd(0xB7); dat(0xC6);
    cmd(0xC0); dat(0x80); dat(0x45);
    cmd(0xC1); dat(0x13);
    cmd(0xC2); dat(0xA7);
    cmd(0xC5); dat(0x0A);

    cmd(0xE8);
    for (uint8_t v : std::array<uint8_t, 8>{0x40,0x8A,0x00,0x00,0x29,0x19,0xA5,0x33}) dat(v);

    cmd(0xE0);
    for (uint8_t v : std::array<uint8_t, 14>{0xD0,0x08,0x0F,0x06,0x06,0x33,0x30,0x33,0x47,0x17,0x13,0x13,0x2B,0x31}) dat(v);

    cmd(0xE1);
    for (uint8_t v : std::array<uint8_t, 14>{0xD0,0x0A,0x11,0x0B,0x09,0x07,0x2F,0x33,0x47,0x38,0x15,0x16,0x2C,0x32}) dat(v);

    cmd(0xF0); dat(0x3C);
    cmd(0xF0); dat(0x69);

    cmd(0x21); // inversion on
    cmd(0x11); usleep(100000);
    cmd(0x29); // display on
}

} // namespace st7796