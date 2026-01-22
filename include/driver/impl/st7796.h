#ifndef CYCOM_DRIVER_IMPL_ST7796_H_
#define CYCOM_DRIVER_IMPL_ST7796_H_

#include <cstdint>
#include <cstddef>
#include <memory>
#include <string>

#include "driver/interface/i_display.h"
#include "hal/interface/i_gpio.h"
#include "hal/interface/i_spi.h"

namespace driver {

/**
 * @brief ST7796 LCDコントローラドライバ実装
 * 
 * IDisplayインターフェースを実装し、ST7796チップを制御します。
 * HAL層のインターフェースを使用してハードウェアに依存しない設計。
 */
class ST7796 : public IDisplay {
public:
    // 画面サイズ
    static constexpr int kWidth = 320;
    static constexpr int kHeight = 480;

    /**
     * @brief ST7796ディスプレイを初期化する
     * 
     * @param spi SPIインターフェース
     * @param dc データ/コマンド選択GPIO
     * @param rst リセットGPIO
     * @param bl バックライトGPIO
     */
    ST7796(hal::ISpi* spi, hal::IGpio* dc, hal::IGpio* rst, hal::IGpio* bl);

    ~ST7796() override = default;

    // IDisplayインターフェースの実装
    void Clear(uint16_t rgb565 = 0xFFFF) override;
    void DrawRGB565Line(int x, int y, const uint16_t* rgb565, int len) override;
    bool DrawBackgroundImage(const std::string& path) override;
    int GetWidth() const override { return kWidth; }
    int GetHeight() const override { return kHeight; }

    /**
     * @brief 矩形領域を指定色で塗りつぶす
     * 
     * @param x0 開始X座標
     * @param y0 開始Y座標
     * @param x1 終了X座標
     * @param y1 終了Y座標
     * @param rgb565 塗りつぶし色
     */
    void DrawFilledRect(int x0, int y0, int x1, int y1, uint16_t rgb565);

    /**
     * @brief RGB565フレームバッファを画面全体に描画する
     * 
     * @param buf RGB565ピクセルデータ
     * @param len バイト数
     */
    void BlitRGB565(const uint8_t* buf, size_t len);

private:
    void DataMode(bool data);
    void Cmd(uint8_t c);
    void Dat(uint8_t d);
    void Reset();
    void SetAddressWindow(int xs, int ys, int xe, int ye);
    void SendChunked(const uint8_t* data, size_t len);
    void Init();

    hal::ISpi* spi_;
    hal::IGpio* dc_;
    hal::IGpio* rst_;
    hal::IGpio* bl_;
};

}  // namespace driver

#endif  // CYCOM_DRIVER_IMPL_ST7796_H_
