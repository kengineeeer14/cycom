#include <cstdint>  // uint8_t, uint16_t
#include <string>   // std::string
#include <vector>   // std::vector
#include <sstream>
#include <mutex>
#include <limits>

#ifndef GPS_L76K_H
#define GPS_L76K_H

namespace sensor_uart{
    
    class L76k{
        public:
            // 受信データ
            struct GNRMC {
                GNRMC()
                    : hour(0),
                      minute(0),
                      second(std::numeric_limits<double>::quiet_NaN()),
                      data_status('V'),
                      latitude(std::numeric_limits<double>::quiet_NaN()),
                      lat_dir('\0'),
                      longitude(std::numeric_limits<double>::quiet_NaN()),
                      lon_dir('\0'),
                      speed_knots(std::numeric_limits<double>::quiet_NaN()),
                      track_deg(std::numeric_limits<double>::quiet_NaN()),
                      date(0),
                      mag_variation(std::numeric_limits<double>::quiet_NaN()),
                      mag_variation_dir('\0'),
                      mode('N'),
                      navigation_status('V'),
                      checksum(0)
                {}
                uint8_t hour;           // UTC時刻: 時
                uint8_t minute;         // UTC時刻: 分
                double second;          // UTC時刻: 秒
                char data_status;       // 'A' = 有効, 'V' = 無効
                double latitude;        // 緯度．dddmm.mmmm
                char lat_dir;           // 'N' or 'S'
                double longitude;       // 経度．dddmm.mmmm
                char lon_dir;           // 'E' or 'W'
                double speed_knots;     // 対地速度[knot]
                double track_deg;       // 移動の真方位[deg]
                uint32_t date;          // UTC日付．ddmmyy形式．
                double mag_variation;   // 磁気偏角[deg]．地北と真北との差．
                char mag_variation_dir; // 磁気偏角の方向；E or W
                char mode;              // モード：N=測位不能、E=デッドレコニング、A=単独測位、D=DGPS、F=RTK float、R=RTK fix
                char navigation_status; // V: 利用不可能．
                uint8_t checksum;       // 1チェックサム
            };

            struct GNVTG {
                GNVTG()
                    : true_track_deg(std::numeric_limits<double>::quiet_NaN()),
                      true_track_indicator('\0'),
                      magnetic_track_deg(std::numeric_limits<double>::quiet_NaN()),
                      magnetic_track_indicator('\0'),
                      speed_knots(std::numeric_limits<double>::quiet_NaN()),
                      speed_knots_unit('\0'),
                      speed_kmh(std::numeric_limits<double>::quiet_NaN()),
                      speed_kmh_unit('\0'),
                      mode('N'),
                      checksum(0)
                {}
                double true_track_deg;      // 真方位 [0.0 - 359.9deg]
                char true_track_indicator; // 'T'（True courseの意味）
                double magnetic_track_deg; // 磁方位 [0.0 - 359.9deg]
                char magnetic_track_indicator; // 'M'（Magnetic courseの意味）
                double speed_knots;       // 地上速度 [knot]
                char speed_knots_unit;    // 'N'（ノットの単位）
                double speed_kmh;         // 地上速度 [km/h]
                char speed_kmh_unit;      // 'K'（km/hの単位）
                char mode;                // モード: 'N'=データなし, 'A'=自律, 'D'=DGPS, 'E'=推定
                uint8_t checksum;         // チェックサム
            };

            struct GNGGA {
                GNGGA()
                    : hour(0),
                      minute(0),
                      second(std::numeric_limits<double>::quiet_NaN()),
                      latitude(std::numeric_limits<double>::quiet_NaN()),
                      lat_dir('\0'),
                      longitude(std::numeric_limits<double>::quiet_NaN()),
                      lon_dir('\0'),
                      quality(0),
                      num_satellites(0),
                      hdop(std::numeric_limits<double>::quiet_NaN()),
                      altitude(std::numeric_limits<double>::quiet_NaN()),
                      altitude_unit('\0'),
                      geoid_height(std::numeric_limits<double>::quiet_NaN()),
                      geoid_unit('\0'),
                      dgps_age(std::numeric_limits<double>::quiet_NaN()),
                      dgps_id(),
                      checksum(0)
                {}
                uint8_t hour;           // UTC時刻: 時
                uint8_t minute;         // UTC時刻: 分
                double second;          // UTC時刻: 秒
                double latitude;        // 緯度．dddmm.mmmm
                char lat_dir;           // 'N' or 'S'
                double longitude;       // 経度．dddmm.mmmm
                char lon_dir;           // 'E' or 'W'
                uint8_t quality;        // 測位品質：0=無効, 1=SPS, 2=DGPS, 4=RTK Fix, 5=RTK Float, etc.
                uint8_t num_satellites; // 使用衛星数
                double hdop;            // 水平精度低下率．衛星配置による精度劣化の垂直成分．
                double altitude;        // アンテナ海抜高度
                char altitude_unit;     // アンテナ海抜高度の単位．通常 'M'．
                double geoid_height;    // ジオイド高さ
                char geoid_unit;        // ジオイド高さの単位．通常 'M'
                double dgps_age;        // 最後に補正情報を受信してからの経過時間[sec]
                std::string dgps_id;    // 補正情報を受け取った基準局ID
                uint8_t checksum;       // チェックサム
            };

            struct GnssSnapshot {
                GNRMC gnrmc;
                GNVTG gnvtg;
                GNGGA gngga;
            };

            void ProcessNmeaLine(const std::string &line);

            GnssSnapshot Snapshot() const;

        private:
            mutable std::mutex mtx_;
            GNRMC gnrmc_data_{};
            GNGGA gngga_data_{};
            GNVTG gnvtg_data_{};

            std::vector<std::string> SplitString(const std::string &line);

            void ParseGnrmc(const std::string &nmea, GNRMC &gnrmc);

            void ParseGnvtg(const std::string &nmea, GNVTG &gnvtg);

            void ParseGngga(const std::string &nmea, GNGGA &gngga);
    };

}   // namespace sensor_uart

#endif // GPS_L76K_H
