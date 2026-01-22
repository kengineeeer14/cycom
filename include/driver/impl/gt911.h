#ifndef CYCOM_DRIVER_IMPL_GT911_H_
#define CYCOM_DRIVER_IMPL_GT911_H_

#include <atomic>
#include <cstdint>
#include <thread>
#include "driver/interface/i_touch.h"
#include "hal/interface/i_i2c.h"
#include "hal/interface/i_gpio.h"

namespace driver {

/**
 * @brief GT911タッチコントローラドライバ実装
 * 
 * ITouchインターフェースを実装し、GT911チップを制御します。
 * HAL層のインターフェースを使用してハードウェアに依存しない設計。
 */
class GT911 : public ITouch {
public:
    // 画面サイズ
    static constexpr int kCoordinateXMax = 320;
    static constexpr int kCoordinateYMax = 480;

    /**
     * @brief GT911タッチコントローラを初期化する
     * 
     * @param i2c I2Cインターフェース
     * @param rst リセットGPIO
     * @param int_pin 割り込みGPIO
     * @param i2c_addr I2Cアドレス (デフォルト: 0x5D)
     */
    GT911(hal::II2c* i2c, hal::IGpio* rst, hal::IGpio* int_pin, uint8_t i2c_addr = 0x5D);

    ~GT911() override;

    // ITouchインターフェースの実装
    TouchPoint GetTouchPoint() override;
    bool IsTouched() override;

private:
    void Reset();
    void ConfigureResolution(int x_max, int y_max);
    void IrqLoop();
    void HandleTouch();
    void ClearStatus();

    hal::II2c* i2c_;
    hal::IGpio* rst_;
    hal::IGpio* int_pin_;
    uint8_t i2c_addr_;

    std::thread th_;
    std::atomic<bool> running_{false};
    std::atomic<int> last_x_{-1};
    std::atomic<int> last_y_{-1};

    // GT911レジスタアドレス
    static constexpr uint16_t COMMAND_REG = 0x8040;
    static constexpr uint16_t X_OUTPUT_MAX_LOW_REG = 0x8048;
    static constexpr uint16_t X_OUTPUT_MAX_HIGH_REG = 0x8049;
    static constexpr uint16_t Y_OUTPUT_MAX_LOW_REG = 0x804A;
    static constexpr uint16_t Y_OUTPUT_MAX_HIGH_REG = 0x804B;
    static constexpr uint16_t COORDINATE_INFO_REG = 0x814E;
    static constexpr uint16_t POINT_1_X_LOW_REG = 0x8150;
    static constexpr uint16_t POINT_1_X_HIGH_REG = 0x8151;
    static constexpr uint16_t POINT_1_Y_LOW_REG = 0x8152;
    static constexpr uint16_t POINT_1_Y_HIGH_REG = 0x8153;
};

}  // namespace driver

#endif  // CYCOM_DRIVER_IMPL_GT911_H_
