#ifndef CYCOM_DRIVER_INTERFACE_I_DISPLAY_H_
#define CYCOM_DRIVER_INTERFACE_I_DISPLAY_H_

#include <cstdint>
#include <string>

namespace driver {

/**
 * @brief ディスプレイドライバの抽象インターフェース
 * 
 * ディスプレイデバイスに依存しない描画操作のインターフェース。
 * テスト時にはモック実装を、本番環境では実デバイス実装を使用する。
 */
class IDisplay {
public:
    virtual ~IDisplay() = default;

    /**
     * @brief 画面全体を指定色でクリアする
     * 
     * @param rgb565 16ビットRGB565フォーマットの色
     */
    virtual void Clear(uint16_t rgb565 = 0xFFFF) = 0;

    /**
     * @brief 指定位置に1行分のRGB565ピクセルデータを描画する
     * 
     * @param x 開始X座標
     * @param y Y座標
     * @param rgb565 RGB565ピクセルデータの配列
     * @param len ピクセル数
     */
    virtual void DrawRGB565Line(int x, int y, const uint16_t* rgb565, int len) = 0;

    /**
     * @brief 背景画像を描画する
     * 
     * @param path 画像ファイルのパス
     * @return true 成功
     * @return false 失敗
     */
    virtual bool DrawBackgroundImage(const std::string& path) = 0;

    /**
     * @brief 画面の幅を取得する
     * 
     * @return int 画面幅（ピクセル）
     */
    virtual int GetWidth() const = 0;

    /**
     * @brief 画面の高さを取得する
     * 
     * @return int 画面高さ（ピクセル）
     */
    virtual int GetHeight() const = 0;
};

}  // namespace driver

#endif  // CYCOM_DRIVER_INTERFACE_I_DISPLAY_H_
