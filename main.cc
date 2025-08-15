#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <iomanip>
#include <cmath>
#include <string>
#include "gpio/gpio_config.h"
#include "sensor/uart/uart_config.h"
#include "sensor/uart/gps/gps_l76k.h"
#include "util/logger.h"

int main() {
    const std::string config_path = "config/config.json";

    gpio::GpioConfigure gpio_config;
    sensor_uart::UartConfigure uart_config(config_path);
    sensor_uart::L76k gps;

    if (!gpio_config.SetupGpio()) {
        return 1;
    }

    int fd = uart_config.SetupUart();
    if (fd < 0) {
        return 1;
    }

    util::Logger logger;
    logger.Start(std::chrono::milliseconds(100));

    char buf[256];
    std::string nmea_line;

    while (true) {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';  // 文字列の最後を示すためにヌル終端を入れる
            nmea_line.append(buf, n);

            // GPSのデータ取得
            size_t pos;
            // 一回のreadで複数のNMEA文をまとめて受信しても安全に処理．ex.)nmea_line: "$GNRMC,....\n$GNGGA,....\n"
            while ((pos = nmea_line.find('\n')) != std::string::npos) {
                std::string line = nmea_line.substr(0, pos + 1); // 改行文字も含めて取得
                gps.ProcessNmeaLine(line);
                nmea_line.erase(0, pos + 1);
            }
        }
    }

    close(fd);
    return 0;
}