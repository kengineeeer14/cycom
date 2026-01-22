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
    std::unique_ptr<hal::GpioImpl> display_dc = std::make_unique<hal::GpioImpl>("gpiochip0", 22, true, 1);
    std::unique_ptr<hal::GpioImpl> display_rst = std::make_unique<hal::GpioImpl>("gpiochip0", 27, true, 1);
    std::unique_ptr<hal::GpioImpl> display_bl = std::make_unique<hal::GpioImpl>("gpiochip0", 18, true, 1);
    
    // SPI: ディスプレイ通信用
    std::unique_ptr<hal::SpiImpl> spi = std::make_unique<hal::SpiImpl>("/dev/spidev0.0", 40000000, 0, 8);
    
    // GPIO: タッチコントローラ用
    std::unique_ptr<hal::GpioImpl> touch_rst = std::make_unique<hal::GpioImpl>("gpiochip0", 1, true, 1);
    std::unique_ptr<hal::GpioImpl> touch_int = std::make_unique<hal::GpioImpl>("gpiochip0", 4, false);
    
    // I2C: タッチコントローラ通信用
    uint8_t gt911_addr = 0x5D;  // GT911のI2Cアドレス
    std::unique_ptr<hal::I2cImpl> i2c = std::make_unique<hal::I2cImpl>("/dev/i2c-1", gt911_addr);
    
    // UART: GPS通信用
    std::unique_ptr<hal::UartImpl> uart = std::make_unique<hal::UartImpl>("/dev/ttyS0", 9600);
    int uart_fd = uart->GetFileDescriptor();
    if (uart_fd < 0) {
        std::cerr << "UART open failed.\n";
        return 1;
    }

    // ========================================
    // ドライバ層のインスタンス生成（HAL層を注入）
    // ========================================
    
    // ディスプレイドライバ
    std::unique_ptr<driver::ST7796> display = std::make_unique<driver::ST7796>(
        spi.get(),
        display_dc.get(),
        display_rst.get(),
        display_bl.get()
    );
    
    // タッチドライバ（Touchスレッドを起動）
    std::unique_ptr<driver::GT911> touch = std::make_unique<driver::GT911>(
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
    
    // ロガー（Loggerスレッドを起動）
    util::Logger logger(config_path, gps);
    
    // センサーマネージャー（Sensorスレッドを起動）
    sensor::SensorManager sensor_manager(uart_fd, gps);
    
    // ディスプレイマネージャー（Displayスレッドを起動）
    display::DisplayManager display_manager(*display, gps);

    // ========================================
    // メインループ（終了シグナル待機のみ）
    // ========================================
    // 
    // 各スレッドの役割:
    // - Sensorスレッド:  GPS L76K からのデータ受信とパース（100msタイムアウト）
    // - Loggerスレッド:  GPSデータのCSV記録（log_interval_ms 周期）
    // - Displayスレッド: UI更新（1秒周期）
    // - Touchスレッド:   タッチ入力監視（イベント駆動）
    // 
    std::cout << "All threads started. Press Ctrl+C to exit.\n";
    
    while (!g_shutdown_requested.load()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Shutting down...\n";
    return 0;
}