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
    Reset();
    Init();
    bl_.Set(1);  // バックライトON
}

void Display::Clear(const uint16_t &rgb565) {
    SetAddressWindow(0, 0, kWidth - 1, kHeight - 1);
    DataMode(true);
    std::vector<uint8_t> line(kWidth * 2);
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            line[2 * x + 0] = static_cast<uint8_t>((rgb565 >> 8) & 0xFF);
            line[2 * x + 1] = static_cast<uint8_t>( rgb565        & 0xFF);
        }
        SendChunked(line.data(), line.size());
    }
}

void Display::DrawFilledRect(int &x0, int &y0, int &x1, int &y1, const uint16_t &rgb565) {
    if (x0 > x1) std::swap(x0, x1);
    if (y0 > y1) std::swap(y0, y1);
    x0 = std::max(0, x0); y0 = std::max(0, y0);
    x1 = std::min(kWidth  - 1, x1);
    y1 = std::min(kHeight - 1, y1);
    if (x0 > x1 || y0 > y1) return;

    SetAddressWindow(x0, y0, x1, y1);
    DataMode(true);

    const int w = (x1 - x0 + 1);
    std::vector<uint8_t> line(w * 2);
    for (int i = 0; i < w; ++i) {
        line[2*i+0] = static_cast<uint8_t>((rgb565 >> 8) & 0xFF);
        line[2*i+1] = static_cast<uint8_t>( rgb565        & 0xFF);
    }
    for (int y = y0; y <= y1; ++y) {
        SendChunked(line.data(), line.size());
    }
}

void Display::BlitRGB565(const uint8_t* buf, const size_t &len) {
    const size_t expected = static_cast<size_t>(kWidth) * kHeight * 2;
    if (len != expected) throw std::runtime_error("BlitRGB565: size mismatch");
    SetAddressWindow(0, 0, kWidth - 1, kHeight - 1);
    DataMode(true);
    SendChunked(buf, len);
}

void st7796::Display::DrawRGB565Line(const int &x, const int &y, const uint16_t* rgb565, const int &len) {
    if (len <= 0) return;
    // アドレスウィンドウを1ラインに絞る
    SetAddressWindow(x, y, x + len - 1, y);
    DataMode(true);

    // ST7796 は 16bit を MSB→LSB の順で送る想定
    // SendChunked(uint8_t* , size_t) がある前提で、バイト配列に詰め替える
    std::vector<uint8_t> bytes;
    bytes.resize(static_cast<size_t>(len) * 2);
    for (int i = 0; i < len; ++i) {
        uint16_t p = rgb565[i];
        bytes[2*i + 0] = static_cast<uint8_t>((p >> 8) & 0xFF);
        bytes[2*i + 1] = static_cast<uint8_t>( p       & 0xFF);
    }
    SendChunked(bytes.data(), bytes.size());
}

void Display::DataMode(bool data) {
    dc_.Set(data ? 1 : 0);
}

void Display::Cmd(uint8_t c) {
    DataMode(false);
    spi_.WriteBytes(&c, 1);
}

void Display::Dat(uint8_t d) {
    DataMode(true);
    spi_.WriteBytes(&d, 1);
}

void Display::Reset() {
    rst_.Set(1); usleep(10000);
    rst_.Set(0); usleep(10000);
    rst_.Set(1); usleep(10000);
}

void Display::SetAddressWindow(const int &xs, const int &ys, const int &xe, const int &ye) {
    // CASET (0x2A): X
    Cmd(0x2A);
    Dat(static_cast<uint8_t>((xs >> 8) & 0xFF)); Dat(static_cast<uint8_t>(xs & 0xFF));
    Dat(static_cast<uint8_t>((xe >> 8) & 0xFF)); Dat(static_cast<uint8_t>(xe & 0xFF));
    // RASET (0x2B): Y
    Cmd(0x2B);
    Dat(static_cast<uint8_t>((ys >> 8) & 0xFF)); Dat(static_cast<uint8_t>(ys & 0xFF));
    Dat(static_cast<uint8_t>((ye >> 8) & 0xFF)); Dat(static_cast<uint8_t>(ye & 0xFF));
    // RAMWR
    Cmd(0x2C);
}

void Display::SendChunked(const uint8_t* data, const size_t &len) {
    const size_t CHUNK = 4096;
    size_t off = 0;
    while (off < len) {
        const size_t n = std::min(CHUNK, len - off);
        spi_.WriteBytes(data + off, n);
        off += n;
    }
}

void Display::Init() {
    // スリープ解除
    Cmd(0x11); usleep(120000);

    // 画面のメモリアクセス制御 (MADCTL) – 向きは好みで
    Cmd(0x36); Dat(0x48);

    // ピクセルフォーマット（16bpp = RGB565）
    Cmd(0x3A); Dat(0x55);

    // 電源制御・フレームレート等のレジスタ設定(電圧制御やパネルドライブの細かいチューニング)
    Cmd(0xF0); Dat(0xC3);
    Cmd(0xF0); Dat(0x96);
    Cmd(0xB4); Dat(0x01);
    Cmd(0xB7); Dat(0xC6);
    Cmd(0xC0); Dat(0x80); Dat(0x45);
    Cmd(0xC1); Dat(0x13);
    Cmd(0xC2); Dat(0xA7);
    Cmd(0xC5); Dat(0x0A);

    // 追加設定（ガンマ補正など）
    Cmd(0xE8);
    for (uint8_t v : std::array<uint8_t, 8>{0x40,0x8A,0x00,0x00,0x29,0x19,0xA5,0x33}) Dat(v);
    Cmd(0xE0);
    for (uint8_t v : std::array<uint8_t, 14>{0xD0,0x08,0x0F,0x06,0x06,0x33,0x30,0x33,0x47,0x17,0x13,0x13,0x2B,0x31}) Dat(v);
    Cmd(0xE1);
    for (uint8_t v : std::array<uint8_t, 14>{0xD0,0x0A,0x11,0x0B,0x09,0x07,0x2F,0x33,0x47,0x38,0x15,0x16,0x2C,0x32}) Dat(v);

    // コマンド保護解除（上で書き込んだ設定を確定させるためのシーケンス）
    Cmd(0xF0); Dat(0x3C);
    Cmd(0xF0); Dat(0x69);

    // 表示設定
    Cmd(0x21); // inversion on
    Cmd(0x11); usleep(100000);
    Cmd(0x29); // display on
}

} // namespace st7796