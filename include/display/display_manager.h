#ifndef DISPLAY_DISPLAY_MANAGER_H
#define DISPLAY_DISPLAY_MANAGER_H

#include <atomic>
#include <thread>
#include "display/st7796.h"
#include "display/text_renderer.h"
#include "sensor/gps/gps_l76k.h"

namespace display {

/**
 * @brief ディスプレイ更新を管理するクラス（Touch / Logger / SensorManager と同じパターン）
 * 
 * コンストラクタでディスプレイ更新スレッドを自動起動し、デストラクタで安全に停止する。
 * 1秒周期でGPSデータを取得し、LCD画面に速度を表示する。
 */
class DisplayManager {
public:
    /**
     * @brief DisplayManager を初期化し、ディスプレイ更新スレッドを自動起動する
     * 
     * @param lcd LCD ディスプレイへの参照
     * @param gps GPS データソースへの参照
     */
    DisplayManager(st7796::Display& lcd, sensor::L76k& gps);

    /**
     * @brief ディスプレイ更新スレッドを安全に停止させる
     */
    ~DisplayManager();

private:
    void Start();
    void Stop();
    
    /**
     * @brief ディスプレイ更新ループ（1秒周期でGPS速度を画面表示）
     */
    void DisplayLoop();

    st7796::Display& lcd_;
    sensor::L76k& gps_;
    ui::TextRenderer tr_;
    
    std::thread th_;
    std::atomic<bool> running_{false};
};

} // namespace display

#endif // DISPLAY_DISPLAY_MANAGER_H
