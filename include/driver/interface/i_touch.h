#ifndef CYCOM_DRIVER_INTERFACE_I_TOUCH_H_
#define CYCOM_DRIVER_INTERFACE_I_TOUCH_H_

#include <utility>

namespace driver {

/**
 * @brief タッチ座標データ
 */
struct TouchPoint {
    int x;
    int y;
    bool touched;  // タッチされているか
};

/**
 * @brief タッチコントローラドライバの抽象インターフェース
 * 
 * タッチデバイスに依存しないタッチ操作のインターフェース。
 * テスト時にはモック実装を、本番環境では実デバイス実装を使用する。
 */
class ITouch {
public:
    virtual ~ITouch() = default;

    /**
     * @brief 現在のタッチ座標を取得する
     * 
     * @return TouchPoint タッチ座標データ
     */
    virtual TouchPoint GetTouchPoint() = 0;

    /**
     * @brief タッチイベントが発生しているか確認する
     * 
     * @return true タッチされている
     * @return false タッチされていない
     */
    virtual bool IsTouched() = 0;
};

}  // namespace driver

#endif  // CYCOM_DRIVER_INTERFACE_I_TOUCH_H_
