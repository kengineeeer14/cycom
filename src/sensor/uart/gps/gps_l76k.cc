#include <iostream>
#include <iomanip>
#include <cmath>
#include "sensor/uart/gps/gps_l76k.h"

namespace sensor_uart{

    std::vector<std::string> L76k::SplitString(const std::string &line) {
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, ',')) {
            fields.push_back(item);
        }
        return fields;
    }

    void L76k::ParseGnrmc(const std::string &nmea, GNRMC &gnrmc) {
        std::vector<std::string> fields = SplitString(nmea);

        // UTC時刻
        if (fields.size() > 1 && fields[1].size() >= 6) {
            gnrmc.hour   = std::stoi(fields[1].substr(0, 2));
            gnrmc.minute = std::stoi(fields[1].substr(2, 2));
            gnrmc.second = std::stod(fields[1].substr(4));
        } else {
            gnrmc.hour = UINT8_MAX;
            gnrmc.minute = UINT8_MAX;
            gnrmc.second = std::numeric_limits<double>::quiet_NaN();
        }
        gnrmc.data_status = (fields.size() > 2 && !fields[2].empty()) ? fields[2][0] : '\0';
        gnrmc.latitude    = (fields.size() > 3 && !fields[3].empty()) ? std::stod(fields[3]) : std::numeric_limits<double>::quiet_NaN();
        gnrmc.lat_dir     = (fields.size() > 4 && !fields[4].empty()) ? fields[4][0] : '\0';
        gnrmc.longitude   = (fields.size() > 5 && !fields[5].empty()) ? std::stod(fields[5]) : std::numeric_limits<double>::quiet_NaN();
        gnrmc.lon_dir     = (fields.size() > 6 && !fields[6].empty()) ? fields[6][0] : '\0';
        gnrmc.speed_knots = (fields.size() > 7 && !fields[7].empty()) ? std::stod(fields[7]) : std::numeric_limits<double>::quiet_NaN();
        gnrmc.track_deg   = (fields.size() > 8 && !fields[8].empty()) ? std::stod(fields[8]) : std::numeric_limits<double>::quiet_NaN();
        gnrmc.date        = (fields.size() > 9 && !fields[9].empty()) ? static_cast<uint32_t>(std::stoi(fields[9])) : UINT16_MAX;
        gnrmc.mag_variation = (fields.size() > 10 && !fields[10].empty()) ? std::stod(fields[10]) : std::numeric_limits<double>::quiet_NaN();
        gnrmc.mag_variation_dir = (fields.size() > 11 && !fields[11].empty()) ? fields[11][0] : '\0';
        gnrmc.mode = (fields.size() > 12 && !fields[12].empty()) ? fields[12][0] : '\0';

        // navigation_status と checksum（<nav>*CS のみ扱う）
        gnrmc.navigation_status = (fields.size() > 13 && !fields[13].empty()) ? fields[13][0] : '\0';
        gnrmc.checksum = 0;  // デフォルトは0（無効扱い）

        if (fields.size() > 13) {
            const std::string &f13 = fields[13];
            size_t starPos = f13.find('*');
            if (starPos != std::string::npos && starPos + 1 < f13.size()) {
                gnrmc.checksum = static_cast<uint8_t>(std::stoi(f13.substr(starPos + 1), nullptr, 16));
            }
        }
    }

    void L76k::ParseGnvtg(const std::string &nmea, GNVTG &gnvtg) {
        std::vector<std::string> fields = SplitString(nmea);

        gnvtg.true_track_deg           = (fields.size() > 1 && !fields[1].empty()) ? std::stod(fields[1]) : std::numeric_limits<double>::quiet_NaN();
        gnvtg.true_track_indicator     = (fields.size() > 2 && !fields[2].empty()) ? fields[2][0] : '\0';
        gnvtg.magnetic_track_deg       = (fields.size() > 3 && !fields[3].empty()) ? std::stod(fields[3]) : std::numeric_limits<double>::quiet_NaN();
        gnvtg.magnetic_track_indicator = (fields.size() > 4 && !fields[4].empty()) ? fields[4][0] : '\0';
        gnvtg.speed_knots              = (fields.size() > 5 && !fields[5].empty()) ? std::stod(fields[5]) : std::numeric_limits<double>::quiet_NaN();
        gnvtg.speed_knots_unit         = (fields.size() > 6 && !fields[6].empty()) ? fields[6][0] : '\0';
        gnvtg.speed_kmh                = (fields.size() > 7 && !fields[7].empty()) ? std::stod(fields[7]) : std::numeric_limits<double>::quiet_NaN();
        gnvtg.speed_kmh_unit           = (fields.size() > 8 && !fields[8].empty()) ? fields[8][0] : '\0';
        if (fields.size() > 9 && !fields[9].empty()) {
            const std::string &f9 = fields[9];
            size_t starPos = f9.find('*');
            // Field 9 format: "<mode>*<checksum>". Mode is the first character.
            gnvtg.mode = f9[0];
            if (starPos != std::string::npos && starPos + 1 < f9.size()) {
                gnvtg.checksum = static_cast<uint8_t>(std::stoi(f9.substr(starPos + 1), nullptr, 16));
            }
        }
    }

    void L76k::ParseGngaa(const std::string &nmea, GNGGA &gngaa) {
        std::vector<std::string> fields = SplitString(nmea);

        if (fields.size() > 1 && fields[1].size() >= 6) {
            gngaa.hour   = std::stoi(fields[1].substr(0, 2));
            gngaa.minute = std::stoi(fields[1].substr(2, 2));
            gngaa.second = std::stod(fields[1].substr(4));
        } else {
            gngaa.hour = UINT8_MAX;
            gngaa.minute = UINT8_MAX;
            gngaa.second = std::numeric_limits<double>::quiet_NaN();
        }
        gngaa.latitude      = (fields.size() > 2 && !fields[2].empty()) ? std::stod(fields[2]) : std::numeric_limits<double>::quiet_NaN();
        gngaa.lat_dir       = (fields.size() > 3 && !fields[3].empty()) ? fields[3][0] : '\0';
        gngaa.longitude     = (fields.size() > 4 && !fields[4].empty()) ? std::stod(fields[4]) : std::numeric_limits<double>::quiet_NaN();
        gngaa.lon_dir       = (fields.size() > 5 && !fields[5].empty()) ? fields[5][0] : '\0';
        gngaa.quality       = (fields.size() > 6 && !fields[6].empty()) ? static_cast<uint8_t>(std::stoi(fields[6])) : UINT8_MAX;
        gngaa.num_satellites= (fields.size() > 7 && !fields[7].empty()) ? static_cast<uint8_t>(std::stoi(fields[7])) : UINT8_MAX;
        gngaa.hdop          = (fields.size() > 8 && !fields[8].empty()) ? std::stod(fields[8]) : std::numeric_limits<double>::quiet_NaN();
        gngaa.altitude      = (fields.size() > 9 && !fields[9].empty()) ? std::stod(fields[9]) : std::numeric_limits<double>::quiet_NaN();
        gngaa.altitude_unit = (fields.size() > 10 && !fields[10].empty()) ? fields[10][0] : '\0';
        gngaa.geoid_height  = (fields.size() > 11 && !fields[11].empty()) ? std::stod(fields[11]) : std::numeric_limits<double>::quiet_NaN();
        gngaa.geoid_unit    = (fields.size() > 12 && !fields[12].empty()) ? fields[12][0] : '\0';
        gngaa.dgps_age      = (fields.size() > 13 && !fields[13].empty()) ? std::stod(fields[13]) : std::numeric_limits<double>::quiet_NaN();

        if (fields.size() > 14 && !fields[14].empty()) {
            size_t starPos = fields[14].find('*');
            if (starPos != std::string::npos) {
                gngaa.dgps_id = fields[14].substr(0, starPos);
                if (starPos + 1 < fields[14].size()) {
                    gngaa.checksum = std::stoi(fields[14].substr(starPos + 1), nullptr, 16);
                }
            }
        }
    }
}   // namespace sensor_uart