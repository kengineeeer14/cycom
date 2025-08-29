#ifndef DISPLAY_TOUCH_GT911_H
#define DISPLAY_TOUCH_GT911_H

#include <atomic>
#include <cstdint>
#include <thread>
#include <utility>
#include "hal/i2c_controller.h"
#include "hal/gpio_line.h"

namespace gt911 {

// =============================================================
// GT911 タッチドライバ
// =============================================================

// レジスタ定義
static constexpr uint16_t COMMAND_REG            = 0x8040;
static constexpr uint16_t GT_CFG_VERSION_REG     = 0x8047;
static constexpr uint16_t X_OUTPUT_MAX_LOW_REG   = 0x8048;
static constexpr uint16_t X_OUTPUT_MAX_HIGH_REG  = 0x8049;
static constexpr uint16_t Y_OUTPUT_MAX_LOW_REG   = 0x804A;
static constexpr uint16_t Y_OUTPUT_MAX_HIGH_REG  = 0x804B;
static constexpr uint16_t TOUCH_NUMBER_REG       = 0x804C;
static constexpr uint16_t COORDINATE_INFO_REG    = 0x814E;
static constexpr uint16_t POINT_1_X_LOW_REG      = 0x8150;
static constexpr uint16_t POINT_1_X_HIGH_REG     = 0x8151;
static constexpr uint16_t POINT_1_Y_LOW_REG      = 0x8152;
static constexpr uint16_t POINT_1_Y_HIGH_REG     = 0x8153;
static constexpr uint16_t GT_CFG_START_REG       = 0x8047;
static constexpr uint16_t GT_CHECKSUM_REG        = 0x80FF;
static constexpr uint16_t GT_FLASH_SAVE_REG      = 0x8100;

// 画面サイズ
static constexpr int COORDINATE_X_MAX = 320;
static constexpr int COORDINATE_Y_MAX = 480;

// GPIO ピン番号(BCM)
static constexpr unsigned kGT911_INT_PIN = 4;
static constexpr unsigned kGT911_RST_PIN = 1;

// =============================================================
// Touch クラス宣言
// =============================================================
class Touch {
public:
    Touch(uint8_t i2c_addr = 0x5D);
    ~Touch();

    // 最新座標を返す（無効時 {-1,-1}）
    std::pair<int,int> lastXY() const;

private:
    void reset();
    void configureResolution(int x_max, int y_max);
    void irqLoop();
    void handleTouch();
    void clearStatus();

private:
    uint8_t   i2c_addr_;
    I2C       i2c_;
    gpio::GpioLine  rst_;
    gpio::GpioLine  int_in_;
    bool      use_events_;
    std::atomic<bool> running_;
    std::thread       th_;
    std::atomic<int>  last_x_, last_y_;
};

} // namespace gt911

#endif // DISPLAY_TOUCH_GT911_H