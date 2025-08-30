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
    /**
     * @brief GT911タッチコントローラを初期化して、イベント(orポーリング)で読み取り続けるスレッドを立ち上げる
     * 
     * @param i2c_addr GT911 タッチコントローラの I2C アドレス
     */
    Touch(const uint8_t &i2c_addr = 0x5D);

    /**
     * @brief タッチ監視スレッドを安全に停止させる
     * 
     */
    ~Touch();

    /**
     * @brief 内部変数に格納されたタッチ座標を返す（無効時は{-1,-1}）
     * 
     * @return std::pair<int,int> タッチ座標
     */
    std::pair<int,int> LastXY() const;

private:
    /**
     * @brief GT911 タッチコントローラをリセットして、所望の I2C アドレスで再起動させる
     * 
     */
    void Reset();

    /**
     * @brief GT911 に対して、画面の解像度（X, Y 最大座標値）を設定する
     * 
     * @param[in] x_max X軸方向の最大座標値（画面の横解像度に相当）
     * @param[in] y_max Y軸方向の最大座標値（画面の縦解像度に相当）
     */
    void ConfigureResolution(const int &x_max, const int &y_max);

    /**
     * @brief タッチコントローラからの入力を監視し、イベントが来たら座標を処理する（内部変数に格納する）
     * 
     */
    void IrqLoop();

    /**
     * @brief GT911 タッチコントローラからタッチ座標を読み取り、加工して保持する
     * 
     */
    void HandleTouch();

    /**
     * @brief  GT911 タッチコントローラの「座標情報レジスタ」をクリアする
     * 
     */
    void ClearStatus();

private:
    uint8_t   i2c_addr_;
    hal::I2C       i2c_;
    hal::GpioLine  rst_;
    hal::GpioLine  int_in_;
    bool      use_events_;
    std::atomic<bool> running_;
    std::thread       th_;
    std::atomic<int>  last_x_, last_y_;
};

} // namespace gt911

#endif // DISPLAY_TOUCH_GT911_H