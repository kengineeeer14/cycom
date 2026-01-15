#include "util/logger.h"
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <fstream>
#include <chrono>
#include <sstream>
#include <nlohmann/json.hpp>

namespace util {
namespace {
    using clock_type = std::chrono::steady_clock;
}

Logger::Logger(const std::string &config_path, sensor::L76k& gps)
    : gps_(gps) {
    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open config file");
    }
    nlohmann::json j;
    ifs >> j;

    log_interval_ms_ = j["logger"]["log_interval_ms"].get<unsigned int>();
    log_on_ = j["logger"]["log_on"].get<bool>();

    csv_file_path_ = GenerateCsvFilePath();
    if (log_on_) {
        WriteLogHeader();
    }
    
    // Touch クラスと同様、コンストラクタで自動的にスレッドを起動
    Start();
}

std::string Logger::GenerateCsvFilePath() {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&now_c);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S") << "_log.csv";
    std::string timestamped_filename = oss.str();

    return (std::filesystem::current_path() / "log" / timestamped_filename).string();
}

Logger::~Logger() { Stop(); }

void Logger::Start() {
    Stop(); // 既存スレッドが動いていれば停止
    running_.store(true, std::memory_order_release);
    th_ = std::thread([this]{ LoggingLoop(); });
}

void Logger::Stop() {
    bool was_running = running_.exchange(false, std::memory_order_acq_rel);
    if (was_running && th_.joinable()) th_.join();
}

void Logger::LoggingLoop() {
    auto next = clock_type::now();
    const auto period = std::chrono::milliseconds(log_interval_ms_);
    
    while (running_.load(std::memory_order_acquire)) {
        // GPS データを取得してログに書き込む（Touch クラスのパターンと同様）
        sensor::GnssSnapshot snap = gps_.Snapshot();
        LogData log_data{snap.gnrmc, snap.gnvtg, snap.gngga};
        if (log_on_) {
            WriteCsv(log_data);
        }
        
        next += period;
        std::this_thread::sleep_until(next);
    }
}

void Logger::WriteLogHeader() {
    std::ofstream ofs(csv_file_path_, std::ios::app);

    ofs
        // RMC (GNRMC)
        << "gnrmc.hour" << ','
        << "gnrmc.minute" << ','
        << "gnrmc.second" << ','
        << "gnrmc.data_status" << ','
        << "gnrmc.latitude" << ','
        << "gnrmc.lat_dir" << ','
        << "gnrmc.longitude" << ','
        << "gnrmc.lon_dir" << ','
        << "gnrmc.speed_knots" << ','
        << "gnrmc.track_deg" << ','
        << "gnrmc.date" << ','
        << "gnrmc.mag_variation" << ','
        << "gnrmc.mag_variation_dir" << ','
        << "gnrmc.mode" << ','
        << "gnrmc.navigation_status" << ','
        << "gnrmc.checksum" << ','
        // VTG (GNVTG)
        << "gnvtg.true_track_deg" << ','
        << "gnvtg.true_track_indicator" << ','
        << "gnvtg.magnetic_track_deg" << ','
        << "gnvtg.magnetic_track_indicator" << ','
        << "gnvtg.speed_knots" << ','
        << "gnvtg.speed_knots_unit" << ','
        << "gnvtg.speed_kmh" << ','
        << "gnvtg.speed_kmh_unit" << ','
        << "gnvtg.mode" << ','
        << "gnvtg.checksum" << ','
        // GGA (GNGGA)
        << "gngga.hour" << ','
        << "gngga.minute" << ','
        << "gngga.second" << ','
        << "gngga.latitude" << ','
        << "gngga.lat_dir" << ','
        << "gngga.longitude" << ','
        << "gngga.lon_dir" << ','
        << "gngga.quality" << ','
        << "gngga.num_satellites" << ','
        << "gngga.hdop" << ','
        << "gngga.altitude" << ','
        << "gngga.altitude_unit" << ','
        << "gngga.geoid_height" << ','
        << "gngga.geoid_unit" << ','
        << "gngga.dgps_age" << ','
        << "gngga.dgps_id" << ','
        << "gngga.checksum"
        << '\n';
}

void Logger::WriteCsv(const LogData &log_data){
    static std::mutex mtx;  // guard against concurrent writes
    std::lock_guard<std::mutex> lk(mtx);

    std::ofstream ofs(csv_file_path_, std::ios::app);
    if (!ofs.is_open()) {
        throw std::runtime_error("Logger::WriteCsv: failed to open CSV file");
    }

    // Row
    ofs
        // RMC
        << static_cast<int>(log_data.gnrmc.hour) << ','
        << static_cast<int>(log_data.gnrmc.minute) << ','
        << log_data.gnrmc.second << ','
        << log_data.gnrmc.data_status << ','
        << log_data.gnrmc.latitude << ','
        << log_data.gnrmc.lat_dir << ','
        << log_data.gnrmc.longitude << ','
        << log_data.gnrmc.lon_dir << ','
        << log_data.gnrmc.speed_knots << ','
        << log_data.gnrmc.track_deg << ','
        << log_data.gnrmc.date << ','
        << log_data.gnrmc.mag_variation << ','
        << log_data.gnrmc.mag_variation_dir << ','
        << log_data.gnrmc.mode << ','
        << log_data.gnrmc.navigation_status << ','
        << static_cast<int>(log_data.gnrmc.checksum) << ','
        // VTG
        << log_data.gnvtg.true_track_deg << ','
        << log_data.gnvtg.true_track_indicator << ','
        << log_data.gnvtg.magnetic_track_deg << ','
        << log_data.gnvtg.magnetic_track_indicator << ','
        << log_data.gnvtg.speed_knots << ','
        << log_data.gnvtg.speed_knots_unit << ','
        << log_data.gnvtg.speed_kmh << ','
        << log_data.gnvtg.speed_kmh_unit << ','
        << log_data.gnvtg.mode << ','
        << static_cast<int>(log_data.gnvtg.checksum) << ','
        // GGA
        << static_cast<int>(log_data.gngga.hour) << ','
        << static_cast<int>(log_data.gngga.minute) << ','
        << log_data.gngga.second << ','
        << log_data.gngga.latitude << ','
        << log_data.gngga.lat_dir << ','
        << log_data.gngga.longitude << ','
        << log_data.gngga.lon_dir << ','
        << static_cast<int>(log_data.gngga.quality) << ','
        << static_cast<int>(log_data.gngga.num_satellites) << ','
        << log_data.gngga.hdop << ','
        << log_data.gngga.altitude << ','
        << log_data.gngga.altitude_unit << ','
        << log_data.gngga.geoid_height << ','
        << log_data.gngga.geoid_unit << ','
        << log_data.gngga.dgps_age << ','
        << log_data.gngga.dgps_id << ','
        << static_cast<int>(log_data.gngga.checksum)
        << '\n';
}

} // namespace util