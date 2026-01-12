#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <csignal>

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
#include "util/shutdown_flag.h"

#include "display/st7796.h"
#include "display/touch/gt911.h"
#include "display/text_renderer.h"

int main() {
    const std::string config_path = "config/config.json";

    // --- シグナルハンドラの設定 ---
    std::signal(SIGINT, [](int) { util::g_shutdown_requested.store(true); });
    std::signal(SIGTERM, [](int) { util::g_shutdown_requested.store(true); });

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

    if (!lcd.DrawBackgroundImage("resource/background/start.jpg")) {
        lcd.Clear(0xFFFF);  // 失敗時は白でフォールバック
    }
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    if (!lcd.DrawBackgroundImage("resource/background/measure.jpg")) {
        lcd.Clear(0xFFFF);  // 失敗時は白でフォールバック
    }

    // ★ フォント描画のセットアップ
    ui::TextRenderer tr(lcd, "config/fonts/DejaVuSans.ttf"); // フォントパスは配置に合わせて
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
        while (!util::g_shutdown_requested.load()) {
            // タイムアウト付きread（select使用）
            fd_set readfds;
            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 100000; // 100ms
            FD_ZERO(&readfds);
            FD_SET(fd, &readfds);
            
            int ret = select(fd + 1, &readfds, nullptr, nullptr, &tv);
            if (ret > 0) {
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
                } else if (n < 0) {
                    std::cerr << "UART read error\n";
                    break;
                }
            } else if (ret < 0) {
                std::cerr << "select() error\n";
                break;
            }
        }
    });

    // --- メインスレッド: 50ms周期でタッチを描画 ---
// --- メインスレッド: 数字+単位を表示 ---
try {
    // 参照渡しAPI向けの薄いラッパ（const値でも安全に渡せる）
    auto FillRect = [&](int x0, int y0, int x1, int y1, uint16_t color) {
        int _x0 = x0, _y0 = y0, _x1 = x1, _y1 = y1;
        lcd.DrawFilledRect(_x0, _y0, _x1, _y1, color);
    };

    // パネル定義
    const int PANEL_X = 20;
    const int PANEL_Y = 40;
    const int PANEL_W = 440;
    const int PANEL_H = 120;

    // --- 単位エリア（従来のレイアウトは維持／テキストは描かない） ---
    tr.SetFontSizePx(28);
    tr.SetColors(ui::Color565::Black(), ui::Color565::White());

    const int UNIT_W = 120;
    const int UNIT_X = PANEL_X + PANEL_W - UNIT_W;
    const int UNIT_Y = PANEL_Y;
    const int UNIT_H = PANEL_H;

    // 単位領域は背景で一度クリアしておく（テキストは描かない）
    // FillRect(UNIT_X, UNIT_Y, UNIT_X + UNIT_W - 1, UNIT_Y + UNIT_H - 1, 0xFFFF);
    // 境界線をうっすら見せたい場合（任意）
    // FillRect(UNIT_X - 1, PANEL_Y, UNIT_X - 1, PANEL_Y + PANEL_H - 1, 0x0000);

    // 数字エリア（レイアウトは従来どおり、UNIT_X を基準に幅を決定）
    const int NUM_X = PANEL_X + 40;
    const int NUM_Y = PANEL_Y + 340;
    const int NUM_W = (UNIT_X - 5) - NUM_X;  // ← 単位領域の直前まで
    const int NUM_H = PANEL_H - 20;

    tr.SetFontSizePx(48);
    tr.SetColors(ui::Color565::Black(), ui::Color565::White());

    const auto UPDATE_INTERVAL = std::chrono::milliseconds(1000);
    std::string prev_text;
    double gnvtg_speed_kmh{0.0};
    while (!util::g_shutdown_requested.load()) {
        std::this_thread::sleep_for(UPDATE_INTERVAL);

        char buf[16];
        gnvtg_speed_kmh = gps.GetGnvtgSpeed();
        std::snprintf(buf, sizeof(buf), "%.1f", gnvtg_speed_kmh);
        std::string cur_text(buf);

        if (cur_text != prev_text) {
            tr.SetWrapWidthPx(0);
            tr.DrawLabel(NUM_X, NUM_Y, NUM_W, NUM_H, cur_text, /*center=*/false);
            prev_text = cur_text;
        }
    }
} catch (const std::exception& e) {
    std::cerr << "Fatal: " << e.what() << "\n";
}

    // --- 終了処理 ---
    std::cout << "\nShutting down...\n";
    
    // ロガースレッドは自動停止（デストラクタ呼び出し）
    // タッチスレッドも自動停止（デストラクタ呼び出し）
    
    // UARTスレッドの終了を待つ
    if (uart_thread.joinable()) {
        uart_thread.join();
    }
    
    ::close(fd);
    std::cout << "Shutdown complete.\n";
    return 0;
}