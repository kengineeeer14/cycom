#include "display/touch/touch_manager.h"
#include <chrono>
#include <iostream>

namespace display {

TouchManager::TouchManager(driver::ITouch& touch)
    : touch_(touch) {
    // Logger / SensorManager / DisplayManager と同様、コンストラクタで自動的にスレッドを起動
    Start();
}

TouchManager::~TouchManager() {
    Stop();
}

void TouchManager::Start() {
    Stop(); // 既存スレッドが動いていれば停止
    running_.store(true, std::memory_order_release);
    th_ = std::thread([this]{ TouchLoop(); });
}

void TouchManager::Stop() {
    bool was_running = running_.exchange(false, std::memory_order_acq_rel);
    if (was_running && th_.joinable()) {
        th_.join();
    }
}

driver::TouchPoint TouchManager::GetLastTouchPoint() const {
    driver::TouchPoint point;
    point.x = last_x_.load(std::memory_order_acquire);
    point.y = last_y_.load(std::memory_order_acquire);
    point.touched = (point.x >= 0 && point.y >= 0);
    return point;
}

bool TouchManager::IsTouched() const {
    return (last_x_.load(std::memory_order_acquire) >= 0 && 
            last_y_.load(std::memory_order_acquire) >= 0);
}

void TouchManager::TouchLoop() {
    try {
        // 定期的にタッチコントローラをポーリング
        const auto POLL_INTERVAL = std::chrono::milliseconds(50);
        
        while (running_.load(std::memory_order_acquire)) {
            driver::TouchPoint point = touch_.GetTouchPoint();
            
            if (point.touched) {
                last_x_.store(point.x, std::memory_order_release);
                last_y_.store(point.y, std::memory_order_release);
            } else {
                // タッチされていない場合は座標をクリア
                last_x_.store(-1, std::memory_order_release);
                last_y_.store(-1, std::memory_order_release);
            }
            
            std::this_thread::sleep_for(POLL_INTERVAL);
        }
    } catch (const std::exception& e) {
        std::cerr << "TouchManager Fatal: " << e.what() << "\n";
    }
}

} // namespace display
