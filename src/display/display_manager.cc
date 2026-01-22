#include "display/display_manager.h"
#include <chrono>
#include <cstdio>
#include <iostream>

namespace display {

DisplayManager::DisplayManager(driver::IDisplay& lcd, sensor::L76k& gps)
    : lcd_(lcd), gps_(gps), tr_(lcd, "config/fonts/DejaVuSans.ttf") {
    // 初期画面を表示
    ShowInitialScreens();
    // Touch / Logger / SensorManager と同様、コンストラクタで自動的にスレッドを起動
    Start();
}

DisplayManager::~DisplayManager() {
    Stop();
}

void DisplayManager::Start() {
    Stop(); // 既存スレッドが動いていれば停止
    running_.store(true, std::memory_order_release);
    th_ = std::thread([this]{ DisplayLoop(); });
}

void DisplayManager::Stop() {
    bool was_running = running_.exchange(false, std::memory_order_acq_rel);
    if (was_running && th_.joinable()) {
        th_.join();
    }
}

void DisplayManager::ShowInitialScreens() {
    // 起動画面を表示
    if (!lcd_.DrawBackgroundImage("resource/background/start.jpg")) {
        lcd_.Clear(0xFFFF);  // 失敗時は白でフォールバック
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    // 計測画面を表示
    if (!lcd_.DrawBackgroundImage("resource/background/measure.jpg")) {
        lcd_.Clear(0xFFFF);  // 失敗時は白でフォールバック
    }
}

void DisplayManager::DisplayLoop() {
    try {
        // パネル定義
        const int PANEL_X = 20;
        const int PANEL_Y = 40;
        const int PANEL_W = 440;
        const int PANEL_H = 120;

        // 単位エリア
        tr_.SetFontSizePx(28);
        tr_.SetColors(ui::Color565::Black(), ui::Color565::White());

        const int UNIT_W = 120;
        const int UNIT_X = PANEL_X + PANEL_W - UNIT_W;
        const int UNIT_Y = PANEL_Y;
        const int UNIT_H = PANEL_H;

        // 数字エリア
        const int NUM_X = PANEL_X + 40;
        const int NUM_Y = PANEL_Y + 340;
        const int NUM_W = (UNIT_X - 5) - NUM_X;
        const int NUM_H = PANEL_H - 20;

        tr_.SetFontSizePx(48);
        tr_.SetColors(ui::Color565::Black(), ui::Color565::White());

        const auto UPDATE_INTERVAL = std::chrono::milliseconds(1000);
        std::string prev_text;
        
        while (running_.load(std::memory_order_acquire)) {
            std::this_thread::sleep_for(UPDATE_INTERVAL);

            char buf[16];
            double gnvtg_speed_kmh = gps_.GetGnvtgSpeed();
            std::snprintf(buf, sizeof(buf), "%.1f", gnvtg_speed_kmh);
            std::string cur_text(buf);

            if (cur_text != prev_text) {
                tr_.SetWrapWidthPx(0);
                tr_.DrawLabel(NUM_X, NUM_Y, NUM_W, NUM_H, cur_text, /*center=*/false);
                prev_text = cur_text;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "DisplayManager Fatal: " << e.what() << "\n";
    }
}

} // namespace display
