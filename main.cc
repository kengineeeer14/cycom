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

void process_nmea_line(sensor_uart::L76k &gps, const std::string &line) {
    std::string nmea_line = line;
    if (nmea_line.rfind("$GNRMC", 0) == 0) {
        sensor_uart::L76k::GNRMC rmc{};
        gps.ParseGnrmc(nmea_line, rmc);
        std::cout << "[GNRMC] ";

        // time: hour/minute use UINT8_MAX as sentinel, second uses NaN
        std::cout << "time=";
        if (rmc.hour != UINT8_MAX && rmc.minute != UINT8_MAX && std::isfinite(rmc.second)) {
            std::cout << std::setw(2) << std::setfill('0') << static_cast<int>(rmc.hour)
                      << ":" << std::setw(2) << static_cast<int>(rmc.minute)
                      << ":" << std::fixed << std::setprecision(2) << rmc.second;
        }

        // status (optional char)
        std::cout << " status=";
        if (rmc.data_status != '\0') std::cout << rmc.data_status;

        // latitude (optional)
        std::cout << " lat=";
        if (!std::isnan(rmc.latitude)) {
            std::cout << std::fixed << std::setprecision(6) << rmc.latitude;
            if (rmc.lat_dir != '\0') std::cout << rmc.lat_dir;
        }

        // longitude (optional)
        std::cout << " lon=";
        if (!std::isnan(rmc.longitude)) {
            std::cout << std::fixed << std::setprecision(6) << rmc.longitude;
            if (rmc.lon_dir != '\0') std::cout << rmc.lon_dir;
        }

        // speed in knots (optional)
        std::cout << " spd(kn)=";
        if (!std::isnan(rmc.speed_knots)) {
            std::cout << std::fixed << std::setprecision(2) << rmc.speed_knots;
        }

        // track (optional)
        std::cout << " track=";
        if (!std::isnan(rmc.track_deg)) {
            std::cout << std::fixed << std::setprecision(2) << rmc.track_deg;
        }

        // date (optional, UINT16_MAX sentinel)
        std::cout << " date=";
        if (rmc.date != UINT16_MAX) {
            std::cout << rmc.date;
        }

        // magnetic variation (optional)
        std::cout << " magVar=";
        if (!std::isnan(rmc.mag_variation)) {
            std::cout << std::fixed << std::setprecision(2) << rmc.mag_variation;
            if (rmc.mag_variation_dir != '\0') std::cout << rmc.mag_variation_dir;
        }

        // mode (optional)
        std::cout << " mode=";
        if (rmc.mode != '\0') std::cout << rmc.mode;

        // navigation status (optional)
        std::cout << " nav=";
        if (rmc.navigation_status != '\0') std::cout << rmc.navigation_status;

        // checksum (uint8_t -> hex 2 digits)
        std::cout << " checksum="
                  << std::uppercase << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<int>(rmc.checksum)
                  << std::dec << std::setfill(' ') << "\n";
    }
    if (nmea_line.rfind("$GNGGA", 0) == 0) {
        sensor_uart::L76k::GNGGA gga{};
        gps.ParseGngaa(nmea_line, gga);
        // std::cout << "[GNGGA] quality: " << static_cast<int>(gga.quality)
        //             << " num_satellites: " << static_cast<int>(gga.num_satellites)
        //             << " dgps_id: " << gga.dgps_id << "\n";
    }
    if (nmea_line.rfind("$GNVTG", 0) == 0) {
        sensor_uart::L76k::GNVTG vtg{};
        gps.ParseGnvtg(nmea_line, vtg);
        // std::cout << "[GNVTG] True Track: " << vtg.true_track_deg << vtg.true_track_indicator
        //             << " Speed(knots): " << vtg.speed_knots
        //             << " Speed(km/h): " << vtg.speed_kmh << "\n";
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