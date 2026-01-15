#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <string>
#include <csignal>
#include <atomic>

#include "hal/gpio_controller.h"
#include "hal/uart_config.h"
#include "sensor/gps/gps_l76k.h"
#include "sensor/sensor_manager.h"
#include "util/logger.h"

#include "display/st7796.h"
#include "display/display_manager.h"
#include "display/touch/gt911.h"

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

    hal::GpioController gpio_controller;
    hal::UartConfigure uart_config(config_path);

    if (!gpio_controller.SetupGpio()) {
        std::cerr << "GPIO setup failed.\n";
        return 1;
    }

    int fd = uart_config.SetupUart();
    if (fd < 0) {
        std::cerr << "UART open failed.\n";
        return 1;
    }

    sensor::L76k gps;
    util::Logger logger(config_path, gps);

    // --- Sensor スレッド起動 ---
    sensor::SensorManager sensor_manager(fd, gps);

    // --- 追加: LCD 初期化＆描画 ---
    st7796::Display lcd;

    // TODO : 画像描画は他のクラスに移す
    if (!lcd.DrawBackgroundImage("resource/background/start.jpg")) {
        lcd.Clear(0xFFFF);  // 失敗時は白でフォールバック
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    if (!lcd.DrawBackgroundImage("resource/background/measure.jpg")) {
        lcd.Clear(0xFFFF);  // 失敗時は白でフォールバック
    }

    // --- 追加: タッチ初期化 ---
    // 既知のGT911アドレス候補は 0x14 / 0x5D。まずは 0x14 を既定に。
    // 必要ならここを 0x5D に変える．
    uint8_t gt911_addr = 0x14;
    // --- Touch スレッド起動 ---
    gt911::Touch touch(gt911_addr);

    // --- Display スレッド起動 ---
    display::DisplayManager display_manager(lcd, gps);

    while (!g_shutdown_requested.load()) {
        // CPUの無駄な消費を防ぐために少し待機
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    std::cout << "Shutting down...\n";

    ::close(fd);
    return 0;
}