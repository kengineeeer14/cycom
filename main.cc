#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <csignal>
#include <atomic>
#include <memory>

// HAL層実装
#include "hal/impl/gpio_impl.h"
#include "hal/impl/spi_impl.h"
#include "hal/impl/i2c_impl.h"
#include "hal/impl/uart_impl.h"

// ドライバ層実装
#include "driver/impl/st7796.h"
#include "driver/impl/gt911.h"

// アプリケーション層（一時的に既存コードを使用）
#include "sensor/gps/gps_l76k.h"
#include "sensor/sensor_manager.h"
#include "util/logger.h"
#include "display/display_manager.h"

namespace {
    std::atomic<bool> g_shutdown_requested{false};
    
    void SignalHandler(int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            g_shutdown_requested.store(true);
        }
    }
}

int main() {
    const std::string config_path = "config/config.json";

    // シグナルハンドラを設定（Ctrl+C で終了可能にする）
    std::signal(SIGINT, SignalHandler);
    std::signal(SIGTERM, SignalHandler);

    // ========================================
    // HAL層のインスタンス生成（依存性注入の準備）
    // ========================================
    
    // GPIO: ディスプレイ制御用
    auto display_dc = std::make_unique<hal::GpioImpl>("gpiochip0", 22, true, 1);
    auto display_rst = std::make_unique<hal::GpioImpl>("gpiochip0", 27, true, 1);
    auto display_bl = std::make_unique<hal::GpioImpl>("gpiochip0", 18, true, 1);
    
    // SPI: ディスプレイ通信用
    auto spi = std::make_unique<hal::SpiImpl>("/dev/spidev0.0", 40000000, 0, 8);
    
    // GPIO: タッチコントローラ用
    auto touch_rst = std::make_unique<hal::GpioImpl>("gpiochip0", 1, true, 1);
    auto touch_int = std::make_unique<hal::GpioImpl>("gpiochip0", 4, false);
    
    // I2C: タッチコントローラ通信用
    uint8_t gt911_addr = 0x5D;  // GT911のI2Cアドレス
    auto i2c = std::make_unique<hal::I2cImpl>("/dev/i2c-1", gt911_addr);
    
    // UART: GPS通信用
    auto uart = std::make_unique<hal::UartImpl>("/dev/ttyS0", 9600);
    int uart_fd = uart->GetFileDescriptor();
    if (uart_fd < 0) {
        std::cerr << "UART open failed.\n";
        return 1;
    }

    // ========================================
    // ドライバ層のインスタンス生成（HAL層を注入）
    // ========================================
    
    // ディスプレイドライバ
    auto display = std::make_unique<driver::ST7796>(
        spi.get(),
        display_dc.get(),
        display_rst.get(),
        display_bl.get()
    );
    
    // タッチドライバ
    auto touch = std::make_unique<driver::GT911>(
        i2c.get(),
        touch_rst.get(),
        touch_int.get(),
        gt911_addr
    );
    
    // GPSドライバ（既存実装を使用）
    sensor::L76k gps;
    
    // ========================================
    // アプリケーション層
    // ========================================
    
    // ロガー
    util::Logger logger(config_path, gps);
    
    // センサーマネージャー
    sensor::SensorManager sensor_manager(uart_fd, gps);
    
    // ディスプレイマネージャー（一時的に既存実装を使用）
    // 注: DisplayManagerは既存実装なので後で修正が必要
    // display::DisplayManager display_manager(*display, gps);

    // メインループ
    while (!g_shutdown_requested.load()) {
        // 簡易的な表示更新（DisplayManagerがないため一時的な実装）
        display->Clear(0x0000);  // 黒でクリア
        
        // タッチ確認
        auto touch_point = touch->GetTouchPoint();
        if (touch_point.touched) {
            std::cout << "Touch: (" << touch_point.x << ", " << touch_point.y << ")\n";
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Shutting down...\n";
    return 0;
}