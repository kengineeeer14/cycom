#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "gpio/gpio_config.h"
#include "sensor/uart/uart_config.h"
#include "sensor/uart/gps/gps_l76k.h"

int main() {
    const std::string config_path = "config/config.json";

    gpio::GpioConfigure gpio_config;
    sensor_uart::UartConfigure uart_config(config_path);
    sensor_uart::L76k gps;   // インスタンス作成

    // GPIOの設定
    if (!gpio_config.SetupGpio()) {
        return 1;
    }

    // UARTの設定
    int fd = uart_config.SetupUart();
    if (fd < 0) {
        return 1;
    }

    char buf[2560];
    std::string nmea_line;

    while (true) {
        int n = read(fd, buf, sizeof(buf) - 1); // 読み取ったバイト数を取得
        if (n > 0) {
            buf[n] = '\0';  // 文字列の終わりを判別できるようにヌル終端を追加
            for (int i = 0; i < n; i++) {
                char c = buf[i];
                if (c == '\n') {
                    // NMEA 1行が揃ったら処理
                    if (!nmea_line.empty() && nmea_line.back() == '\r') {
                        nmea_line.pop_back(); // \r削除
                    }

                    // --- パーサ呼び出し ---
                    if (nmea_line.rfind("$GNRMC", 0) == 0) {
                        sensor_uart::L76k::GNRMC rmc{};
                        if (gps.parseGNRMC(nmea_line, rmc)) {
                            std::cout << "[GNRMC] Lat: " << rmc.latitude << rmc.lat_dir
                                      << " Lon: " << rmc.longitude << rmc.lon_dir
                                      << " Speed(knots): " << rmc.speed_knots
                                      << " Track: " << rmc.track_deg << "\n";
                        }
                    } else if (nmea_line.rfind("$GNGGA", 0) == 0) {
                        sensor_uart::L76k::GNGGA gga{};
                        if (gps.parseGNGGA(nmea_line, gga)) {
                            std::cout << "[GNGGA] quality: " << gga.quality
                                      << " num_satellites: " << gga.num_satellites
                                      << " dgps_id: " << gga.dgps_id << "\n";
                        }
                    }
                    nmea_line.clear(); // 次の行へ
                }
                else {
                    nmea_line.push_back(c);
                }
            }
        }
    }

    close(fd);
    return 0;
}
