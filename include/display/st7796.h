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
    /**
     * @brief GPIOのライン，SPIインターフェース，ディスプレイ設定を初期化
     * 
     */
    Display();


    /**
     * @brief 画面全体を指定した16ビット色で塗りつぶす
     * 
     * @param[in] rgb565 16ビットのカラー値
     */
    void Clear(const uint16_t &rgb565 = 0xFFFF);

    /**
     * @brief 画面の指定した矩形範囲を指定した色で塗りつぶす
     * 
     * @param[in] x0 矩形のx座標始点
     * @param[in] y0 矩形のy座標始点
     * @param[in] x1 矩形のx座標終点
     * @param[in] y1 矩形のy座標終点
     * @param[in] rgb565 塗りつぶす色
     */
    void DrawFilledRect(int &x0, int &y0, int &x1, int &y1, const uint16_t &rgb565);

    /**
     * @brief 渡された フレームバッファ（RGB565 の生データ） を、画面全体にそのまま一括描画する．
     * 
     * @param[in] buf 描画するピクセルデータ（RGB565 形式）の配列
     * @param[in] len buf のバイト数
     */
    void BlitRGB565(const uint8_t* buf, const size_t &len);

    // 既存のヘッダに追記（publicで）
    // 行yの(x .. x+len-1) に RGB565 の1ラインを描画
    // rgb565: len個のピクセル(16bit, RGB565)
    void DrawRGB565Line(const int &x, const int &y, const uint16_t* rgb565, const int &len);

private:
    /**
     * @brief dataがtrueの時にはgpioラインに1を設定し，falseであれば0を設定する．
     * 
     * @param data コマンドモードかデータモードか（false: コマンドモード，true: データモード）
     */
    void DataMode(bool data);

    /**
     * @brief gpioラインをコマンドモードに設定し，spiでコマンドを送る．
     * 
     * @param[in] c 送信するコマンド
     */
    void Cmd(uint8_t c);

    /**
     * @brief gpioラインをデータモードに設定し，spiでデータを送る．
     * 
     * @param[in] d 送信するデータ
     */
    void Dat(uint8_t d);

    /**
     * @brief LCD コントローラをハードウェアリセットする
     * 
     */
    void Reset();

    /**
     * @brief LCD コントローラに対して描画する矩形領域（X・Y の開始座標と終了座標）を設定し、以降のピクセルデータを書き込む準備を行う
     * 
     * @param[in] xs 開始 X 座標（左端）
     * @param[in] ys 開始 Y 座標（上端）
     * @param[in] xe 終了 X 座標（右端）
     * @param[in] ye 終了 Y 座標（下端）
     */
    void SetAddressWindow(const int &xs, const int &ys, const int &xe, const int &ye);

    /**
     * @brief データ全体 (len バイト) を、CHUNK バイトごとに分割して SPI に送信する
     * 
     * @param[in] data 送信したいデータの先頭アドレス
     * @param[in] len 送信するデータの総バイト数
     */
    void SendChunked(const uint8_t* data, const size_t &len);

    /**
     * @brief LCD コントローラのスリープ解除・メモリアクセス制御・ピクセルフォーマット設定・電源/ガンマ調整を行い、最終的に画面を点灯状態にする
     * 
     */
    void Init();

private:
    // 値メンバにして未完成型問題を回避
    hal::GpioLine dc_;
    hal::GpioLine rst_;
    hal::GpioLine bl_;
    hal::SPI      spi_;
};

}  // namespace st7796

#endif  // INCLUDE_DISPLAY_ST7796_H_