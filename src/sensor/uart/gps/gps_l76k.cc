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
        // TODO マジックナンバーの使用を回避

        std::vector<std::string> fields = SplitString(nmea);

        // UTC時刻
        if (fields[1].size() >= 6) {
            gnrmc.hour   = std::stoi(fields[1].substr(0, 2));
            gnrmc.minute = std::stoi(fields[1].substr(2, 2));
            gnrmc.second = std::stod(fields[1].substr(4));
        } else {
            gnrmc.hour = 0;
            gnrmc.minute = 0;
            gnrmc.second = 0.0;
        }
        gnrmc.data_status = fields[2].empty() ? ' ' : fields[2][0];
        gnrmc.latitude    = fields[3].empty() ? 0.0 : std::stod(fields[3]);
        gnrmc.lat_dir     = fields[4].empty() ? ' ' : fields[4][0];
        gnrmc.longitude   = fields[5].empty() ? 0.0 : std::stod(fields[5]);
        gnrmc.lon_dir     = fields[6].empty() ? ' ' : fields[6][0];
        gnrmc.speed_knots = fields[7].empty() ? 0.0 : std::stod(fields[7]);
        gnrmc.track_deg   = fields[8].empty() ? 0.0 : std::stod(fields[8]);
        gnrmc.date        = static_cast<uint16_t>(std::stoi(fields[9]));
        gnrmc.mag_variation = fields[10].empty() ? 0.0 : std::stod(fields[10]);
        gnrmc.mag_variation_dir = fields[11].empty() ? ' ' : fields[11][0];
        gnrmc.mode = fields[12].empty() ? ' ' : fields[12][0];

        // navigation_status と checksum（<nav>*CS のみ扱う）
        gnrmc.navigation_status = (fields.size() > 13 && !fields[13].empty()) ? fields[13][0] : ' ';
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
        // TODO マジックナンバーの使用を回避

        std::vector<std::string> fields = SplitString(nmea);

        gnvtg.true_track_deg         = fields[1].empty() ? 0.0 : std::stod(fields[1]);
        gnvtg.true_track_indicator   = fields[2].empty() ? ' ' : fields[2][0];
        gnvtg.magnetic_track_deg     = fields[3].empty() ? 0.0 : std::stod(fields[3]);
        gnvtg.magnetic_track_indicator = fields[4].empty() ? ' ' : fields[4][0];
        gnvtg.speed_knots            = fields[5].empty() ? 0.0 : std::stod(fields[5]);
        gnvtg.speed_knots_unit       = fields[6].empty() ? ' ' : fields[6][0];
        gnvtg.speed_kmh              = fields[7].empty() ? 0.0 : std::stod(fields[7]);

        if (fields[8].find('*') != std::string::npos) {
            size_t starPos = fields[8].find('*');
            gnvtg.speed_kmh_unit = fields[8][0];
            gnvtg.mode = (starPos > 1) ? fields[8][1] : ' ';
            gnvtg.checksum = std::stoi(fields[8].substr(starPos + 1), nullptr, 16);
        }
    }

    void L76k::ParseGngaa(const std::string &nmea, GNGGA &gngaa) {
        // TODO マジックナンバーの使用を回避

        std::vector<std::string> fields = SplitString(nmea);

        if (fields[1].size() >= 6) {
            gngaa.hour   = std::stoi(fields[1].substr(0, 2));
            gngaa.minute = std::stoi(fields[1].substr(2, 2));
            gngaa.second = std::stod(fields[1].substr(4));
        } else {
            gngaa.hour = 0;
            gngaa.minute = 0;
            gngaa.second = 0.0;
        }
        gngaa.latitude = std::stod(fields[2]);
        gngaa.lat_dir  = fields[3].empty() ? ' ' : fields[3][0];
        gngaa.longitude = std::stod(fields[4]);
        gngaa.lon_dir   = fields[5].empty() ? ' ' : fields[5][0];
        gngaa.quality   = static_cast<uint8_t>(std::stoi(fields[6]));
        gngaa.num_satellites = static_cast<uint8_t>(std::stoi(fields[7]));
        gngaa.hdop      = std::stod(fields[8]);
        gngaa.altitude  = std::stod(fields[9]);
        gngaa.altitude_unit = fields[10].empty() ? ' ' : fields[10][0];
        gngaa.geoid_height  = std::stod(fields[11]);
        gngaa.geoid_unit    = fields[12].empty() ? ' ' : fields[12][0];
        gngaa.dgps_age      = fields[13].empty() ? 0.0 : std::stod(fields[13]);

        if (fields[14].find('*') != std::string::npos) {
            size_t starPos = fields[14].find('*');
            gngaa.dgps_id = fields[14].substr(0, starPos);
            gngaa.checksum = std::stoi(fields[14].substr(starPos + 1), nullptr, 16);
        }
    }

    

}   // namespace sensor_uart