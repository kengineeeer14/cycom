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

#include "display/st7796.h"
#include "display/touch/gt911.h"
#include "display/text_renderer.h"

int main() {
    const std::string config_path = "config/config.json";

    // --- 既存初期化（GPIO / UART / GPS / Logger） ---
    hal::GpioController gpio_controller;
    hal::UartConfigure uart_config(config_path);
    sensor::L76k gps;
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
        sensor::L76k::GnssSnapshot snap = gps.Snapshot();
        util::Logger::LogData log_data{snap.gnrmc, snap.gnvtg, snap.gngga};
        if (logger.log_on_){
            logger.WriteCsv(log_data);
        }
    });

    // --- 追加: LCD 初期化＆テスト描画 ---
    st7796::Display lcd;
    lcd.Clear(0xFFFF); // 白で全消去

    // ★ フォント描画のセットアップ
    ui::TextRenderer tr(lcd, "assets/fonts/NotoSans-Regular.ttf"); // フォントパスは配置に合わせて
    tr.SetFontSizePx(48);
    tr.SetColors(ui::Color565::Black(), ui::Color565::White());

    // --- 追加: タッチ初期化 ---
    // 既知のGT911アドレス候補は 0x14 / 0x5D。まずは 0x14 を既定に。
    // 必要ならここを 0x5D に変える．
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
        const int PANEL_X = 20;
        const int PANEL_Y = 40;
        const int PANEL_W = 420;
        const int PANEL_H = 120;
        tr.SetWrapWidthPx(0); // 折返しなし
        double demo_speed = 23.4; // デモ値（描画確認用）

        int t = 0;
        while (true) {
            // タッチ座標の取得
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            auto xy = touch.LastXY();
            int x = xy.first;
            int y = xy.second;

            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.1f km/h", demo_speed);
            tr.DrawLabel(PANEL_X, PANEL_Y, PANEL_W, PANEL_H, buf, /*center=*/true);
            demo_speed += 0.3; if (demo_speed > 45.0) demo_speed = 18.0;

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