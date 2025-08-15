#include "util/logger.h"
#include <iostream>

namespace util {
namespace {
    using clock_type = std::chrono::steady_clock;
}

Logger::Logger() = default;

Logger::Logger(const std::string &config_path) {
    std::ifstream ifs(config_path);
    if (!ifs.is_open()) {
        throw std::runtime_error("Failed to open config file");
    }
    nlohmann::json j;
    ifs >> j;

    log_interval_ms_ = j["logger"]["log_interval_ms"].get<unsigned int>();
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

} // namespace util