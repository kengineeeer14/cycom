#include "sensor/uart/gps/gps_l76k.h"

namespace sensor_uart{
    L76k::parseGNRMC(const std::string &nmea, GNRMC &out) {

        if (nmea.rfind("$GNRMC", 0) != 0) return false;

        auto fields = split_csv(nmea);
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
        return true
    }

}   // namespace sensor_uart