#include "util/logger.h"
#include <iostream>
#include <filesystem>
#include <iomanip>
#include <mutex>
#include <fstream>

namespace util {
namespace {
    using clock_type = std::chrono::steady_clock;
}

Logger::Logger(const std::string &config_path) {
    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open config file");
    }
    nlohmann::json j;
    ifs >> j;

    log_interval_ms_ = j["logger"]["log_interval_ms"].get<unsigned int>();
    csv_file_path_ = j["logger"]["csv_file_path"].get<std::string>();

    WriteLogHeader();
}

Logger::~Logger() { Stop(); }

void Logger::Start(std::chrono::milliseconds period, Callback cb) {
    Stop(); // stop existing thread if running
    period_ = period;
    cb_ = std::move(cb);
    running_.store(true, std::memory_order_release);
    th_ = std::thread([this]{
        auto next = clock_type::now();
        while (running_.load(std::memory_order_acquire)) {
            if (cb_) cb_(); else OnTick();
            next += period_;
            std::this_thread::sleep_until(next);
        }
    });
}

void Logger::Start(std::chrono::milliseconds period) {
    Start(period, nullptr);
}

void Logger::Stop() {
    bool was_running = running_.exchange(false, std::memory_order_acq_rel);
    if (was_running && th_.joinable()) th_.join();
}

void Logger::OnTick() {
    // Default periodic processing
    std::cout << "100ms\n";
}

void Logger::WriteLogHeader() {
    std::ofstream ofs(csv_file_path_, std::ios::app);

    ofs
        // RMC (GNRMC)
        << "rmc_hour" << ','
        << "rmc_minute" << ','
        << "rmc_second" << ','
        << "rmc_data_status" << ','
        << "rmc_latitude" << ','
        << "rmc_lat_dir" << ','
        << "rmc_longitude" << ','
        << "rmc_lon_dir" << ','
        << "rmc_speed_knots" << ','
        << "rmc_track_deg" << ','
        << "rmc_date" << ','
        << "rmc_mag_variation" << ','
        << "rmc_mag_variation_dir" << ','
        << "rmc_mode" << ','
        << "rmc_navigation_status" << ','
        << "rmc_checksum" << ','
        // VTG (GNVTG)
        << "vtg_true_track_deg" << ','
        << "vtg_true_track_indicator" << ','
        << "vtg_magnetic_track_deg" << ','
        << "vtg_magnetic_track_indicator" << ','
        << "vtg_speed_knots" << ','
        << "vtg_speed_knots_unit" << ','
        << "vtg_speed_kmh" << ','
        << "vtg_speed_kmh_unit" << ','
        << "vtg_mode" << ','
        << "vtg_checksum" << ','
        // GGA (GNGGA)
        << "gga_hour" << ','
        << "gga_minute" << ','
        << "gga_second" << ','
        << "gga_latitude" << ','
        << "gga_lat_dir" << ','
        << "gga_longitude" << ','
        << "gga_lon_dir" << ','
        << "gga_quality" << ','
        << "gga_num_satellites" << ','
        << "gga_hdop" << ','
        << "gga_altitude" << ','
        << "gga_altitude_unit" << ','
        << "gga_geoid_height" << ','
        << "gga_geoid_unit" << ','
        << "gga_dgps_age" << ','
        << "gga_dgps_id" << ','
        << "gga_checksum"
        << '\n';
}

void Logger::WriteCsv(const LogData &log_data){
    static std::mutex mtx;  // guard against concurrent writes
    std::lock_guard<std::mutex> lk(mtx);

    // Ensure parent directory exists and open file
    try {
        std::filesystem::path p(csv_file_path_);
        if (p.has_parent_path() && !p.parent_path().empty()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (...) {
        // Ignore directory creation failures; file open may still work in CWD
    }

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