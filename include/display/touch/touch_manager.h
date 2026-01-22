#ifndef DISPLAY_TOUCH_TOUCH_MANAGER_H
#define DISPLAY_TOUCH_TOUCH_MANAGER_H

#include <atomic>
#include <thread>
#include "driver/interface/i_touch.h"

namespace display {

/**
 * @brief タッチ入力を管理するクラス（Logger / SensorManager / DisplayManager と同じパターン）
 * 
 * コンストラクタでタッチ監視スレッドを自動起動し、デストラクタで安全に停止する。
 * GPIO割り込みイベントでタッチコントローラをポーリングし、タッチ座標を内部に保存する。
 */
class TouchManager {
public:
    /**
     * @brief TouchManager を初期化し、タッチ監視スレッドを自動起動する
     * 
     * @param touch タッチコントローラへの参照
     */
    explicit TouchManager(driver::ITouch& touch);

    /**
     * @brief タッチ監視スレッドを安全に停止させる
     */
    ~TouchManager();

    /**
     * @brief 最後のタッチ座標を取得する
     * 
     * @return TouchPoint タッチ情報（touched=false の場合はタッチされていない）
     */
    driver::TouchPoint GetLastTouchPoint() const;

    /**
     * @brief 現在タッチされているかを取得する
     * 
     * @return bool タッチされている場合はtrue
     */
    bool IsTouched() const;

private:
    void Start();
    void Stop();
    
    /**
     * @brief タッチ監視ループ（GPIO割り込みイベントでタッチを検出）
     */
    void TouchLoop();

    driver::ITouch& touch_;
    
    std::thread th_;
    std::atomic<bool> running_{false};
    std::atomic<int> last_x_{-1};
    std::atomic<int> last_y_{-1};
};

} // namespace display

#endif // DISPLAY_TOUCH_TOUCH_MANAGER_H
