#include "display/touch/gt911.h"
#include <unistd.h>       // usleep
#include <gpiod.h>        // gpiod_line_event

namespace gt911 {

Touch::Touch(uint8_t i2c_addr)
    : i2c_addr_(i2c_addr),
      i2c_("/dev/i2c-1", i2c_addr),
      rst_("gpiochip0", kGT911_RST_PIN, true, 1),
      int_in_("gpiochip0", kGT911_INT_PIN, false),
      use_events_(false),
      running_(true), last_x_(-1), last_y_(-1)
{
    reset();

    // 動作モード一旦停止
    i2c_.write8(COMMAND_REG, 0x02);
    usleep(10000);

    // 解像度設定
    configureResolution(COORDINATE_X_MAX, COORDINATE_Y_MAX);

    // 動作開始
    i2c_.write8(COMMAND_REG, 0x00);
    usleep(50000);

    // 割り込み要求
    try {
        int_in_.RequestFallingEdge();
        use_events_ = true;
    } catch (...) {
        try {
            int_in_.RequestRisingEdge();
            use_events_ = true;
        } catch (...) {
            use_events_ = false;
        }
    }

    // スレッド起動
    th_ = std::thread([this]{ this->irqLoop(); });
}

Touch::~Touch() {
    running_ = false;
    if (th_.joinable()) th_.join();
}

std::pair<int,int> Touch::lastXY() const {
    return { last_x_.load(), last_y_.load() };
}

void Touch::reset() {
    int desired_level = (i2c_addr_ == 0x5D) ? 1 : 0;
    try {
        hal::GpioLine int_force("gpiochip0", kGT911_INT_PIN, true, desired_level);
        rst_.Set(0); usleep(10000);
        rst_.Set(1); usleep(50000);
    } catch (...) {
        rst_.Set(0); usleep(10000);
        rst_.Set(1); usleep(50000);
    }
}

void Touch::configureResolution(int x_max, int y_max) {
    i2c_.write8(X_OUTPUT_MAX_LOW_REG,  x_max & 0xFF);
    i2c_.write8(X_OUTPUT_MAX_HIGH_REG, (x_max >> 8) & 0x0F);
    i2c_.write8(Y_OUTPUT_MAX_LOW_REG,  y_max & 0xFF);
    i2c_.write8(Y_OUTPUT_MAX_HIGH_REG, (y_max >> 8) & 0x0F);
}

void Touch::irqLoop() {
    while (running_) {
        bool handled = false;
        if (use_events_) {
            int r = int_in_.WaitEvent(200);
            if (r > 0) {
                gpiod_line_event ev{};
                try {
                    int_in_.ReadEvent(ev);
                    handleTouch();
                    handled = true;
                } catch (...) {}
            }
        }
        if (!handled) {
            handleTouch();
            usleep(20000);
        }
    }
}

void Touch::handleTouch() {
    uint8_t info = 0;
    try {
        info = i2c_.read8(COORDINATE_INFO_REG);
    } catch (...) { return; }

    uint8_t touch_num = info & 0x0F;
    if (touch_num == 0) {
        last_x_.store(-1);
        last_y_.store(-1);
        clearStatus();
        return;
    }

    uint8_t x_lo=0,x_hi=0,y_lo=0,y_hi=0;
    try {
        x_lo = i2c_.read8(POINT_1_X_LOW_REG);
        x_hi = i2c_.read8(POINT_1_X_HIGH_REG);
        y_lo = i2c_.read8(POINT_1_Y_LOW_REG);
        y_hi = i2c_.read8(POINT_1_Y_HIGH_REG);
    } catch (...) { return; }

    int x = ((int)x_hi << 8) | x_lo;
    int y = ((int)y_hi << 8) | y_lo;

    // 左右反転 + 範囲内にクリップ
    x = COORDINATE_X_MAX - 1 - x;
    if (x < 0) x = 0; else if (x >= COORDINATE_X_MAX) x = COORDINATE_X_MAX - 1;
    if (y < 0) y = 0; else if (y >= COORDINATE_Y_MAX) y = COORDINATE_Y_MAX - 1;

    last_x_.store(x);
    last_y_.store(y);

    clearStatus();
}

void Touch::clearStatus() {
    try {
        i2c_.write8(COORDINATE_INFO_REG, 0x00);
    } catch (...) {}
}

} // namespace gt911