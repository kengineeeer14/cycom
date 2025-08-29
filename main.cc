// main.cc (統合版)
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

#include <chrono>
#include <thread>
#include <iostream>
#include <iomanip>
#include <string>
#include <algorithm>

#include "hal/gpio_controller.h"
#include "hal/uart_config.h"
#include "sensor/gps/gps_l76k.h"
#include "util/logger.h"

// 追加: LCD/タッチ
#include "display/st7796.h"
#include "display/touch/gt911.h"

int main() {
    const std::string config_path = "config/config.json";

    // --- 既存初期化（GPIO / UART / GPS / Logger） ---
    hal::GpioController gpio_controller;
    hal::UartConfigure uart_config(config_path);
    sensor_uart::L76k gps;
    util::Logger logger(config_path);

    if (!gpio_controller.SetupGpio()) {
        std::cerr << "GPIO setup failed.\n";
        return 1;
    }

    int fd = uart_config.SetupUart();
    if (fd < 0) {
        std::cerr << "UART open failed.\n";
        return 1;
    }

    // ロガー起動（既存のコールバック）
    logger.Start(std::chrono::milliseconds(logger.log_interval_ms_), [&] {
        sensor_uart::L76k::GnssSnapshot snap = gps.Snapshot();
        util::Logger::LogData log_data{snap.gnrmc, snap.gnvtg, snap.gngga};
        logger.WriteCsv(log_data);
    });

    // --- 追加: LCD 初期化＆テスト描画 ---
    st7796::Display lcd;
    lcd.clear(0xFFFF); // 白で全消去

    // 6色バーで表示確認
    {
        const uint16_t colors[6] = {0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F};
        const int band = st7796::kHeight / 6;
        for (int i = 0; i < 6; ++i) {
            int y0 = i * band;
            int y1 = (i == 5) ? (st7796::kHeight - 1) : (y0 + band - 1);
            lcd.drawFilledRect(0, y0, st7796::kWidth - 1, y1, colors[i]);
        }
    }

    // --- 追加: タッチ初期化 ---
    // 既知のGT911アドレス候補は 0x14 / 0x5D。まずは 0x14 を既定に。
    // 必要ならここを 0x5D に変えて試してね。
    uint8_t gt911_addr = 0x14;
    gt911::Touch touch(gt911_addr);

    // --- UART読み取りを別スレッドで回す ---
    std::thread uart_thread([&gps, fd]() {
        char buf[256];
        std::string nmea_line;
        while (true) {
            int n = ::read(fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                nmea_line.append(buf, n);

                // 1回のreadで複数文が来ても安全に処理
                size_t pos;
                while ((pos = nmea_line.find('\n')) != std::string::npos) {
                    std::string line = nmea_line.substr(0, pos + 1); // 改行含めて取得
                    gps.ProcessNmeaLine(line);
                    nmea_line.erase(0, pos + 1);
                }
            }
            // エラーハンドリングやタイムアウト等が必要ならここに追加
        }
    });

    // --- メインスレッド: 50ms周期でタッチを描画 ---
    try {
        int t = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            auto xy = touch.lastXY();
            int x = xy.first;
            int y = xy.second;

            if (x >= 0 && y >= 0) {
                // タッチ点を青で可視化（5x5）
                int x1 = std::min(x + 5, st7796::kWidth - 1);
                int y1 = std::min(y + 5, st7796::kHeight - 1);
                lcd.drawFilledRect(x, y, x1, y1, 0x001F);
                std::cout << "Coordinate x=" << x << " y=" << y << "\n";
            } else {
                // ハートビート（左→右に走る白い小矩形）
                int bx0 = (t % st7796::kWidth);
                int bx1 = std::min(bx0 + 10, st7796::kWidth - 1);
                lcd.drawFilledRect(bx0, 0, bx1, 10, 0xFFFF);
                t += 6;
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        // fallthrough
    }

    // 到達しないが一応
    if (uart_thread.joinable()) {
        // 実運用では終了シグナルを用意してからjoinする
        uart_thread.detach();
    }
    ::close(fd);
    return 0;
}