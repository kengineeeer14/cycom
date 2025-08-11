#include "sensor/uart/gps/gps_l76k.h"

namespace sensor_uart{

    std::vector<std::string> L76k::split_string(const std::string &line) {
        std::vector<std::string> fields;
        std::stringstream ss(line);
        std::string item;
        while (std::getline(ss, item, ',')) {
            fields.push_back(item);
        }
        return fields;
    }

    bool L76k::parseGNRMC(const std::string &nmea, GNRMC &out) {

        if (nmea.rfind("$GNRMC", 0) != 0) return false;

        auto fields = split_string(nmea);
        if (fields.size() < 13) return false;

        // UTC時刻
        if (fields[1].size() >= 6) {
            out.hour   = std::stoi(fields[1].substr(0, 2));
            out.minute = std::stoi(fields[1].substr(2, 2));
            out.second = std::stod(fields[1].substr(4));
        }
        out.data_status = fields[2].empty() ? 'V' : fields[2][0];
        out.latitude    = std::stod(fields[3]);
        out.lat_dir     = fields[4].empty() ? ' ' : fields[4][0];
        out.longitude   = std::stod(fields[5]);
        out.lon_dir     = fields[6].empty() ? ' ' : fields[6][0];
        out.speed_knots = std::stod(fields[7]);
        out.track_deg   = std::stod(fields[8]);
        out.date        = static_cast<uint16_t>(std::stoi(fields[9]));
        out.mag_variation = fields[10].empty() ? 0.0 : std::stod(fields[10]);
        out.mag_variation_dir = fields[11].empty() ? ' ' : fields[11][0];

        // モードとナビゲーションステータス＋チェックサム
        if (fields[12].size() > 2 && fields[12].find('*') != std::string::npos) {
            size_t starPos = fields[12].find('*');
            out.mode = fields[12][0];
            out.navigation_status = (starPos > 1) ? fields[12][1] : ' ';
            out.checksum = std::stoi(fields[12].substr(starPos + 1), nullptr, 16);
        }
        return true;
    }

    bool L76k::parseGNVTG(const std::string &nmea, GNVTG &out) {
        if (nmea.rfind("$GNVTG", 0) != 0) return false;

        auto fields = split_string(nmea);
        if (fields.size() < 9) return false;

        out.true_track_deg         = fields[1].empty() ? 0.0 : std::stod(fields[1]);
        out.true_track_indicator   = fields[2].empty() ? ' ' : fields[2][0];
        out.magnetic_track_deg     = fields[3].empty() ? 0.0 : std::stod(fields[3]);
        out.magnetic_track_indicator = fields[4].empty() ? ' ' : fields[4][0];
        out.speed_knots            = fields[5].empty() ? 0.0 : std::stod(fields[5]);
        out.speed_knots_unit       = fields[6].empty() ? ' ' : fields[6][0];
        out.speed_kmh              = fields[7].empty() ? 0.0 : std::stod(fields[7]);

        if (fields[8].find('*') != std::string::npos) {
            size_t starPos = fields[8].find('*');
            out.speed_kmh_unit = fields[8][0];
            out.mode = (starPos > 1) ? fields[8][1] : ' ';
            out.checksum = std::stoi(fields[8].substr(starPos + 1), nullptr, 16);
        }
        return true;
    }

    bool L76k::parseGNGGA(const std::string &nmea, GNGGA &out) {
        if (nmea.rfind("$GNGGA", 0) != 0) return false;

        auto fields = split_string(nmea);
        if (fields.size() < 15) return false;

        if (fields[1].size() >= 6) {
            out.hour   = std::stoi(fields[1].substr(0, 2));
            out.minute = std::stoi(fields[1].substr(2, 2));
            out.second = std::stod(fields[1].substr(4));
        }
        out.latitude = std::stod(fields[2]);
        out.lat_dir  = fields[3].empty() ? ' ' : fields[3][0];
        out.longitude = std::stod(fields[4]);
        out.lon_dir   = fields[5].empty() ? ' ' : fields[5][0];
        out.quality   = static_cast<uint8_t>(std::stoi(fields[6]));
        out.num_satellites = static_cast<uint8_t>(std::stoi(fields[7]));
        out.hdop      = std::stod(fields[8]);
        out.altitude  = std::stod(fields[9]);
        out.altitude_unit = fields[10].empty() ? ' ' : fields[10][0];
        out.geoid_height  = std::stod(fields[11]);
        out.geoid_unit    = fields[12].empty() ? ' ' : fields[12][0];
        out.dgps_age      = fields[13].empty() ? 0.0 : std::stod(fields[13]);

        if (fields[14].find('*') != std::string::npos) {
            size_t starPos = fields[14].find('*');
            out.dgps_id = fields[14].substr(0, starPos);
            out.checksum = std::stoi(fields[14].substr(starPos + 1), nullptr, 16);
        }
        return true;
    }

    

}   // namespace sensor_uart