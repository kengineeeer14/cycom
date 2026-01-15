#include "sensor/sensor_manager.h"
#include <unistd.h>
#include <string>

namespace sensor {

SensorManager::SensorManager(int uart_fd, L76k& gps)
    : uart_fd_(uart_fd), gps_(gps) {
    // Touch / Logger クラスと同様、コンストラクタで自動的にスレッドを起動
    Start();
}

SensorManager::~SensorManager() {
    Stop();
}

void SensorManager::Start() {
    Stop(); // 既存スレッドが動いていれば停止
    running_.store(true, std::memory_order_release);
    th_ = std::thread([this]{ SensorLoop(); });
}

void SensorManager::Stop() {
    bool was_running = running_.exchange(false, std::memory_order_acq_rel);
    if (was_running && th_.joinable()) {
        th_.join();
    }
}

void SensorManager::SensorLoop() {
    char buf[256];
    std::string nmea_line;
    
    while (running_.load(std::memory_order_acquire)) {
        int n = ::read(uart_fd_, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            nmea_line.append(buf, n);

            // 1回のreadで複数文が来ても安全に処理
            size_t pos;
            while ((pos = nmea_line.find('\n')) != std::string::npos) {
                std::string line = nmea_line.substr(0, pos + 1); // 改行含めて取得
                gps_.ProcessNmeaLine(line);
                nmea_line.erase(0, pos + 1);
            }
        }
        // TODO: エラーハンドリングやタイムアウト等が必要ならここに追加
    }
}

} // namespace sensor
