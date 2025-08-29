#ifndef INCLUDE_DISPLAY_ST7796_H_
#define INCLUDE_DISPLAY_ST7796_H_

#include <cstdint>
#include <cstddef>

// ここはあなたの実装ヘッダに合わせて調整
#include "hal/gpio_line.h"
#include "hal/spi_controller.h"

namespace st7796 {

// 画面サイズ
static constexpr int kWidth  = 320;
static constexpr int kHeight = 480;

// ピン（BCM）
static constexpr unsigned kRST = 27;
static constexpr unsigned kDC  = 22;
static constexpr unsigned kBL  = 18;

// -------------------------------------------------------------
// Display
//   ST7796 向けの最小機能ドライバ。
// -------------------------------------------------------------
class Display {
public:
    Display();

    void clear(uint16_t rgb565 = 0xFFFF);
    void drawFilledRect(int x0, int y0, int x1, int y1, uint16_t rgb565);
    void blitRGB565(const uint8_t* buf, size_t len);

private:
    void dataMode(bool data);
    void cmd(uint8_t c);
    void dat(uint8_t d);
    void reset();
    void setAddressWindow(int xs, int ys, int xe, int ye);
    void sendChunked(const uint8_t* data, size_t len);
    void init();

private:
    // 値メンバにして未完成型問題を回避
    GpioLine dc_;
    GpioLine rst_;
    GpioLine bl_;
    SPI      spi_;
};

}  // namespace st7796

#endif  // INCLUDE_DISPLAY_ST7796_H_