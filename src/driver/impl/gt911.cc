#include "driver/impl/gt911.h"
#include <stdexcept>
#include <unistd.h>

namespace driver {

GT911::GT911(hal::II2c* i2c, hal::IGpio* rst, hal::IGpio* int_pin, uint8_t i2c_addr)
    : i2c_(i2c), rst_(rst), int_pin_(int_pin), i2c_addr_(i2c_addr), running_(true) {
    if (!i2c_ || !rst_ || !int_pin_) {
        throw std::invalid_argument("GT911: null pointer provided");
    }

    Reset();

    // 動作モード一旦停止
    uint8_t stop_cmd = 0x02;
    i2c_->Write16(i2c_addr_, COMMAND_REG, &stop_cmd, 1);
    usleep(10000);

    // 解像度設定
    ConfigureResolution(kCoordinateXMax, kCoordinateYMax);

    // 動作開始
    uint8_t start_cmd = 0x00;
    i2c_->Write16(i2c_addr_, COMMAND_REG, &start_cmd, 1);
    usleep(50000);

    // GPIO割り込み要求
    try {
        int_pin_->RequestFallingEdge();
    } catch (...) {
        try {
            int_pin_->RequestRisingEdge();
        } catch (...) {
            // 割り込み設定失敗時はポーリングモードで動作
        }
    }

    // タッチ監視スレッド起動
    th_ = std::thread([this] { this->IrqLoop(); });
}

GT911::~GT911() {
    running_ = false;
    if (th_.joinable()) th_.join();
}

TouchPoint GT911::GetTouchPoint() {
    TouchPoint point;
    point.x = last_x_.load();
    point.y = last_y_.load();
    point.touched = (point.x >= 0 && point.y >= 0);
    return point;
}

bool GT911::IsTouched() {
    return (last_x_.load() >= 0 && last_y_.load() >= 0);
}

void GT911::Reset() {
    int desired_level = (i2c_addr_ == 0x5D) ? 1 : 0;

    // INTピンを一時的に出力モードに設定してアドレスを決定
    // 注: 本来は別のGpioインスタンスが必要だが、簡略化のため現在のピンを使用
    try {
        int_pin_->Set(desired_level);
    } catch (...) {
        // INTピンが出力モードでない場合はスキップ
    }

    rst_->Set(0);
    usleep(10000);
    rst_->Set(1);
    usleep(50000);
}

void GT911::ConfigureResolution(int x_max, int y_max) {
    uint8_t x_lo = x_max & 0xFF;
    uint8_t x_hi = (x_max >> 8) & 0x0F;
    uint8_t y_lo = y_max & 0xFF;
    uint8_t y_hi = (y_max >> 8) & 0x0F;

    i2c_->Write16(i2c_addr_, X_OUTPUT_MAX_LOW_REG, &x_lo, 1);
    i2c_->Write16(i2c_addr_, X_OUTPUT_MAX_HIGH_REG, &x_hi, 1);
    i2c_->Write16(i2c_addr_, Y_OUTPUT_MAX_LOW_REG, &y_lo, 1);
    i2c_->Write16(i2c_addr_, Y_OUTPUT_MAX_HIGH_REG, &y_hi, 1);
}

void GT911::IrqLoop() {
    while (running_) {
        // GPIO割り込みイベントを待機（200ms = 0.2秒タイムアウト）
        bool event_occurred = false;
        try {
            event_occurred = int_pin_->WaitForEvent(1);  // 1秒タイムアウト
        } catch (...) {
            // エラー時は短時間スリープしてリトライ
            usleep(100000);
            continue;
        }

        if (event_occurred) {
            HandleTouch();
        }
    }
}

void GT911::HandleTouch() {
    uint8_t info = 0;
    if (!i2c_->Read16(i2c_addr_, COORDINATE_INFO_REG, &info, 1)) {
        return;
    }

    uint8_t touch_num = info & 0x0F;
    if (touch_num == 0) {
        last_x_.store(-1);
        last_y_.store(-1);
        ClearStatus();
        return;
    }

    uint8_t coords[4] = {0, 0, 0, 0};
    if (!i2c_->Read16(i2c_addr_, POINT_1_X_LOW_REG, coords, 4)) {
        return;
    }

    int x = ((int)coords[1] << 8) | coords[0];
    int y = ((int)coords[3] << 8) | coords[2];

    // 左右反転 + 範囲内にクリップ
    x = kCoordinateXMax - 1 - x;
    if (x < 0) x = 0;
    else if (x >= kCoordinateXMax) x = kCoordinateXMax - 1;
    if (y < 0) y = 0;
    else if (y >= kCoordinateYMax) y = kCoordinateYMax - 1;

    last_x_.store(x);
    last_y_.store(y);

    ClearStatus();
}

void GT911::ClearStatus() {
    uint8_t clear = 0x00;
    i2c_->Write16(i2c_addr_, COORDINATE_INFO_REG, &clear, 1);
}

}  // namespace driver
