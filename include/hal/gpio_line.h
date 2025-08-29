#ifndef CYCOM_HAL_GPIO_LINE_H_
#define CYCOM_HAL_GPIO_LINE_H_

#include <gpiod.h>
#include <string>
#include <stdexcept>
#include <ctime>    // timespec

namespace gpio {

// 単一GPIOラインのRAIIラッパ（libgpiod v1）
class GpioLine {
public:
    // chip は "/dev/gpiochip0" でも "gpiochip0" でもOK
    GpioLine(const std::string& chip, unsigned int offset, bool output, int initial = 0);

    // 入力ラインをエッジイベントにする
    void requestRisingEdge();
    void requestFallingEdge();

    // イベント待ち（ms）: >0=イベント, 0=タイムアウト, <0=エラー
    int waitEvent(int timeout_ms);
    void readEvent(gpiod_line_event* ev);  // 直近イベントを取得

    // 出力ラインに値をセット
    void set(int value);

    // コピー/ムーブ禁止（所有明確化）
    GpioLine(const GpioLine&) = delete;
    GpioLine& operator=(const GpioLine&) = delete;
    GpioLine(GpioLine&&) = delete;
    GpioLine& operator=(GpioLine&&) = delete;

    ~GpioLine();

private:
    static gpiod_chip* OpenChipFlexible(const std::string& chip);

private:
    gpiod_chip* chip_{nullptr};
    gpiod_line* line_{nullptr};
    bool is_output_{false};
};

}  // namespace gpio

#endif  // CYCOM_HAL_GPIO_LINE_H_