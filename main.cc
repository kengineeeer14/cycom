#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "gpio/gpio_config.h"
#include "sensor/uart/uart_config.h"
#include "sensor/uart/gps/gps_l76k.h"

void process_nmea_line(sensor_uart::L76k &gps, const std::string &line) {
    std::string nmea_line = line;
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
            std::cout << "[GNGGA] quality: " << static_cast<int>(gga.quality)
                      << " num_satellites: " << static_cast<int>(gga.num_satellites)
                      << " dgps_id: " << gga.dgps_id << "\n";
        }
    } else if (nmea_line.rfind("$GNVTG", 0) == 0) {
        sensor_uart::L76k::GNVTG vtg{};
        if (gps.parseGNVTG(nmea_line, vtg)) {
            std::cout << "[GNVTG] True Track: " << vtg.true_track_deg << vtg.true_track_indicator
                      << " Speed(knots): " << vtg.speed_knots
                      << " Speed(km/h): " << vtg.speed_kmh << "\n";
        }
    }
}

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

    char buf[256];
    std::string nmea_line;

    while (true) {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';  // 文字列の最後を示すためにヌル終端を入れる
            nmea_line.append(buf, n);

            size_t pos;
            // 一回のreadで複数のNMEA文をまとめて受信しても安全に処理．
            // ex.)nmea_line: "$GNRMC,....\n$GNGGA,....\n"
            while ((pos = nmea_line.find('\n')) != std::string::npos) {
                std::string line = nmea_line.substr(0, pos + 1); // 改行文字も含めて取得
                process_nmea_line(gps, line);
                nmea_line.erase(0, pos + 1);
            }
        }
    }

    close(fd);
    return 0;
}