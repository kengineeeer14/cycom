#include "util/logger.h"
#include <iostream>
#include <filesystem>
#include <iomanip>

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

void Logger::WriteCsv(const LogData &log_data){
    static std::mutex mtx;  // guard against concurrent writes
    std::lock_guard<std::mutex> lk(mtx);

    // Create parent directory if needed
    try {
        std::filesystem::path p(csv_file_path_);
        if (p.has_parent_path() && !p.parent_path().empty()) {
            std::filesystem::create_directories(p.parent_path());
        }
    } catch (...) {
        // ignore directory creation failures; file open may still work in CWD
    }

    std::ofstream ofs(csv_file_path_, std::ios::app);
    if (!ofs.is_open()) {
        throw std::runtime_error("Logger::WriteCsv: failed to open CSV file");
    }

    // Header
    ofs 
        // RMC
        << "rmc_time" << ',' << "rmc_status" << ','
        << "rmc_lat" << ',' << "rmc_lat_hemi" << ','
        << "rmc_lon" << ',' << "rmc_lon_hemi" << ','
        << "rmc_speed_knots" << ',' << "rmc_track_angle" << ','
        << "rmc_date" << ',' << "rmc_mag_var" << ',' << "rmc_mag_var_dir" << ','
        << "rmc_mode" << ',' << "rmc_checksum" << ','
        // VTG
        << "vtg_true_track" << ',' << "vtg_magnetic_track" << ','
        << "vtg_speed_knots" << ',' << "vtg_speed_kmh" << ','
        << "vtg_mode" << ',' << "vtg_checksum" << ','
        // GGA
        << "gga_time" << ','
        << "gga_lat" << ',' << "gga_lat_hemi" << ','
        << "gga_lon" << ',' << "gga_lon_hemi" << ','
        << "gga_fix_quality" << ',' << "gga_num_satellites" << ','
        << "gga_hdop" << ','
        << "gga_altitude" << ',' << "gga_altitude_unit" << ','
        << "gga_geoid_sep" << ',' << "gga_geoid_unit" << ','
        << "gga_dgps_age" << ',' << "gga_dgps_id" << ','
        << "gga_checksum"
        << '\n';

    // Row
    ofs 
        // RMC (fields expected in L76k::GNRMC)
        << log_data.gnrmc.time_utc << ','
        << log_data.gnrmc.status << ','
        << log_data.gnrmc.latitude << ','
        << log_data.gnrmc.lat_hemisphere << ','
        << log_data.gnrmc.longitude << ','
        << log_data.gnrmc.lon_hemisphere << ','
        << log_data.gnrmc.speed_knots << ','
        << log_data.gnrmc.track_angle << ','
        << log_data.gnrmc.date_ddmmyy << ','
        << log_data.gnrmc.magnetic_variation << ','
        << log_data.gnrmc.mag_var_dir << ','
        << log_data.gnrmc.mode << ','
        << log_data.gnrmc.checksum << ','
        // VTG (fields expected in L76k::GNVTG)
        << log_data.gnvtg.true_track << ','
        << log_data.gnvtg.magnetic_track << ','
        << log_data.gnvtg.speed_knots << ','
        << log_data.gnvtg.speed_kmh << ','
        << log_data.gnvtg.mode << ','
        << log_data.gnvtg.checksum << ','
        // GGA (fields expected in L76k::GNGGA)
        << log_data.gngga.time_utc << ','
        << log_data.gngga.latitude << ','
        << log_data.gngga.lat_hemisphere << ','
        << log_data.gngga.longitude << ','
        << log_data.gngga.lon_hemisphere << ','
        << log_data.gngga.fix_quality << ','
        << log_data.gngga.num_satellites << ','
        << log_data.gngga.hdop << ','
        << log_data.gngga.altitude << ','
        << log_data.gngga.altitude_unit << ','
        << log_data.gngga.geoid_separation << ','
        << log_data.gngga.geoid_unit << ','
        << log_data.gngga.dgps_age << ','
        << log_data.gngga.dgps_station_id << ','
        << log_data.gngga.checksum
        << '\n';

    ofs.flush();
}

} // namespace util