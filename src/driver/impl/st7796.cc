#include "driver/impl/st7796.h"
#include "third_party/stb_image.h"
#include <algorithm>
#include <array>
#include <stdexcept>
#include <vector>
#include <unistd.h>
#include <cmath>
#include <cstdio>

namespace driver {

ST7796::ST7796(hal::ISpi* spi, hal::IGpio* dc, hal::IGpio* rst, hal::IGpio* bl)
    : spi_(spi), dc_(dc), rst_(rst), bl_(bl) {
    if (!spi_ || !dc_ || !rst_ || !bl_) {
        throw std::invalid_argument("ST7796: null pointer provided");
    }
    
    // 初期化シーケンス
    Reset();
    Init();
    bl_->Set(1);  // バックライトON
}

void ST7796::Clear(uint16_t rgb565) {
    SetAddressWindow(0, 0, kWidth - 1, kHeight - 1);
    DataMode(true);
    std::vector<uint8_t> line(kWidth * 2);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            line[2 * x + 0] = static_cast<uint8_t>((rgb565 >> 8) & 0xFF);
            line[2 * x + 1] = static_cast<uint8_t>(rgb565 & 0xFF);
        }
        SendChunked(line.data(), line.size());
    }
}

void ST7796::DrawFilledRect(int x0, int y0, int x1, int y1, uint16_t rgb565) {
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    x0 = std::max(0, x0); y0 = std::max(0, y0);
    x1 = std::min(kWidth - 1, x1);
    y1 = std::min(kHeight - 1, y1);
    if (x0 > x1 || y0 > y1) return;

    SetAddressWindow(x0, y0, x1, y1);
    DataMode(true);

    const int w = (x1 - x0 + 1);
    std::vector<uint8_t> line(w * 2);
    for (int i = 0; i < w; ++i) {
        line[2 * i + 0] = static_cast<uint8_t>((rgb565 >> 8) & 0xFF);
        line[2 * i + 1] = static_cast<uint8_t>(rgb565 & 0xFF);
    }
    for (int y = y0; y <= y1; ++y) {
        SendChunked(line.data(), line.size());
    }
}

void ST7796::BlitRGB565(const uint8_t* buf, size_t len) {
    const size_t expected = static_cast<size_t>(kWidth) * kHeight * 2;
    if (len != expected) throw std::runtime_error("BlitRGB565: size mismatch");
    SetAddressWindow(0, 0, kWidth - 1, kHeight - 1);
    DataMode(true);
    SendChunked(buf, len);
}

void ST7796::DrawRGB565Line(int x, int y, const uint16_t* rgb565, int len) {
    if (len <= 0) return;
    SetAddressWindow(x, y, x + len - 1, y);
    DataMode(true);

    std::vector<uint8_t> bytes(static_cast<size_t>(len) * 2);
    for (int i = 0; i < len; ++i) {
        uint16_t p = rgb565[i];
        bytes[2 * i + 0] = static_cast<uint8_t>((p >> 8) & 0xFF);
        bytes[2 * i + 1] = static_cast<uint8_t>(p & 0xFF);
    }
    SendChunked(bytes.data(), bytes.size());
}

bool ST7796::DrawBackgroundImage(const std::string& path) {
    int w, h, ch;
    unsigned char* img = stbi_load(path.c_str(), &w, &h, &ch, 0);
    if (!img) {
        std::fprintf(stderr, "Failed to load background: %s\n", path.c_str());
        return false;
    }
    if (ch < 3) {
        stbi_image_free(img);
        return false;
    }

    std::vector<uint16_t> line(kWidth);

    const double scale = std::max(double(kWidth) / w, double(kHeight) / h);
    const double sw = kWidth / scale;
    const double sh = kHeight / scale;
    const double sx0 = (w - sw) * 0.5;
    const double sy0 = (h - sh) * 0.5;

    for (int y = 0; y < kHeight; ++y) {
        double fy = sy0 + (y + 0.5) / scale;
        int sy = std::clamp(static_cast<int>(std::floor(fy)), 0, h - 1);
        for (int x = 0; x < kWidth; ++x) {
            double fx = sx0 + (x + 0.5) / scale;
            int sx = std::clamp(static_cast<int>(std::floor(fx)), 0, w - 1);
            const unsigned char* p = img + (sy * w + sx) * ch;
            uint16_t c = ((p[0] & 0xF8) << 8) | ((p[1] & 0xFC) << 3) | (p[2] >> 3);
            line[x] = c;
        }
        DrawRGB565Line(0, y, line.data(), kWidth);
    }

    stbi_image_free(img);
    return true;
}

void ST7796::DataMode(bool data) {
    dc_->Set(data ? 1 : 0);
}

void ST7796::Cmd(uint8_t c) {
    DataMode(false);
    spi_->WriteBytes(&c, 1);
}

void ST7796::Dat(uint8_t d) {
    DataMode(true);
    spi_->WriteBytes(&d, 1);
}

void ST7796::Reset() {
    rst_->Set(1); usleep(10000);
    rst_->Set(0); usleep(10000);
    rst_->Set(1); usleep(10000);
}

void ST7796::SetAddressWindow(int xs, int ys, int xe, int ye) {
    // CASET (0x2A): X
    Cmd(0x2A);
    Dat(static_cast<uint8_t>((xs >> 8) & 0xFF));
    Dat(static_cast<uint8_t>(xs & 0xFF));
    Dat(static_cast<uint8_t>((xe >> 8) & 0xFF));
    Dat(static_cast<uint8_t>(xe & 0xFF));
    // RASET (0x2B): Y
    Cmd(0x2B);
    Dat(static_cast<uint8_t>((ys >> 8) & 0xFF));
    Dat(static_cast<uint8_t>(ys & 0xFF));
    Dat(static_cast<uint8_t>((ye >> 8) & 0xFF));
    Dat(static_cast<uint8_t>(ye & 0xFF));
    // RAMWR
    Cmd(0x2C);
}

void ST7796::SendChunked(const uint8_t* data, size_t len) {
    const size_t CHUNK = 4096;
    size_t off = 0;
    while (off < len) {
        const size_t n = std::min(CHUNK, len - off);
        spi_->WriteBytes(data + off, n);
        off += n;
    }
}

void ST7796::Init() {
    // スリープ解除
    Cmd(0x11);
    usleep(120000);

    // メモリアクセス制御 (MADCTL)
    Cmd(0x36);
    Dat(0x48);

    // ピクセルフォーマット（16bpp = RGB565）
    Cmd(0x3A);
    Dat(0x55);

    // 電源制御
    Cmd(0xF0); Dat(0xC3);
    Cmd(0xF0); Dat(0x96);
    Cmd(0xB4); Dat(0x01);
    Cmd(0xB7); Dat(0xC6);
    Cmd(0xC0); Dat(0x80); Dat(0x45);
    Cmd(0xC1); Dat(0x13);
    Cmd(0xC2); Dat(0xA7);
    Cmd(0xC5); Dat(0x0A);

    // ガンマ補正等
    Cmd(0xE8);
    for (uint8_t v : std::array<uint8_t, 8>{0x40, 0x8A, 0x00, 0x00, 0x29, 0x19, 0xA5, 0x33})
        Dat(v);
    Cmd(0xE0);
    for (uint8_t v : std::array<uint8_t, 14>{0xD0, 0x08, 0x0F, 0x06, 0x06, 0x33, 0x30, 0x33,
                                              0x47, 0x17, 0x13, 0x13, 0x2B, 0x31})
        Dat(v);
    Cmd(0xE1);
    for (uint8_t v : std::array<uint8_t, 14>{0xD0, 0x0A, 0x11, 0x0B, 0x09, 0x07, 0x2F, 0x33,
                                              0x47, 0x38, 0x15, 0x16, 0x2C, 0x32})
        Dat(v);

    // コマンド保護解除
    Cmd(0xF0); Dat(0x3C);
    Cmd(0xF0); Dat(0x69);

    // 表示ON
    Cmd(0x21);  // inversion on
    Cmd(0x11);
    usleep(100000);
    Cmd(0x29);  // display on
}

}  // namespace driver
