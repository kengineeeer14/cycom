#ifndef CYCOM_HAL_IMPL_GPIO_IMPL_H_
#define CYCOM_HAL_IMPL_GPIO_IMPL_H_

#include <string>
#include <stdexcept>
#include <ctime>

#include "hal/interface/i_gpio.h"

#ifdef USE_HARDWARE
#include <gpiod.h>
#else
// 開発環境用のモック構造体（gpiodが使えない環境用）
struct gpiod_chip;
struct gpiod_line;
struct gpiod_line_event {
    int event_type;
    timespec ts;
};
#endif

namespace hal {

/**
 * @brief GPIO操作の実装クラス（libgpiod v1使用）
 * 
 * IGpioインターフェースを実装し、Raspberry PiのGPIOピンを制御します。
 * libgpiodライブラリを使用した実ハードウェア実装です。
 */
class GpioImpl : public IGpio {
public:
    /**
     * @brief GPIOピンを初期化する
     * 
     * @param chip GPIOチップの指定（例: "/dev/gpiochip0" or "gpiochip0"）
     * @param offset チップ内のライン番号（GPIO番号に対応）
     * @param output 出力ピンとして扱うかどうか
     * @param initial 出力ピンの初期値 (0=Low, 1=High)
     */
    GpioImpl(const std::string& chip, unsigned int offset, bool output, int initial = 0);

    // コピー/ムーブ禁止
    GpioImpl(const GpioImpl&) = delete;
    GpioImpl& operator=(const GpioImpl&) = delete;
    GpioImpl(GpioImpl&&) = delete;
    GpioImpl& operator=(GpioImpl&&) = delete;

    ~GpioImpl() override;

    // IGpioインターフェースの実装
    void Set(int value) override;
    int Get() override;
    void RequestRisingEdge() override;
    void RequestFallingEdge() override;
    bool WaitForEvent(int timeout_sec) override;

    /**
     * @brief GPIOイベントを読み取る（libgpiod固有の機能）
     * 
     * @param ev イベントデータを格納する構造体
     */
    void ReadEvent(gpiod_line_event& ev);

private:
    /**
     * @brief チップ名からgpiod_chipを開く
     * 
     * @param chip チップパスまたは名前
     * @return gpiod_chip* 開いたチップのハンドル
     */
    static gpiod_chip* OpenChipFlexible(const std::string& chip);

    gpiod_chip* chip_{nullptr};
    gpiod_line* line_{nullptr};
    bool is_output_{false};
    const char* consumer_{"cycom"};
};

}  // namespace hal

#endif  // CYCOM_HAL_IMPL_GPIO_IMPL_H_
