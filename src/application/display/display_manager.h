#ifndef CYCOM_SRC_APPLICATION_DISPLAY_DISPLAY_MANAGER_H_
#define CYCOM_SRC_APPLICATION_DISPLAY_DISPLAY_MANAGER_H_

#include "domain/sensor/gps_l76k.h"
#include "driver/interface/i_display.h"
#include "presentation/display/freetype_font_loader.h"
#include "presentation/display/text_renderer.h"

#include <atomic>
#include <memory>
#include <thread>

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
    DisplayManager(driver::IDisplay &lcd, sensor::L76k &gps);

    /**
     * @brief ディスプレイ更新スレッドを安全に停止させる
     */
    ~DisplayManager();

  private:
    void Start();
    void Stop();

    /**
     * @brief 初期画面を表示する（起動画面 → 計測画面）
     */
    void ShowInitialScreens();

    /**
     * @brief ディスプレイ更新ループ（1秒周期でGPS速度を画面表示）
     */
    void DisplayLoop();

    driver::IDisplay &lcd_;
    sensor::L76k &gps_;
    std::unique_ptr<ui::FreeTypeFontLoader> font_loader_;
    ui::TextRenderer tr_;

    std::thread th_;
    std::atomic<bool> running_{false};
};

}  // namespace display

#endif  // CYCOM_SRC_APPLICATION_DISPLAY_DISPLAY_MANAGER_H_
