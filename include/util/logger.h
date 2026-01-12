#ifndef LOGGER_H
#define LOGGER_H

#include <chrono>
#include <functional>
#include <atomic>
#include <thread>
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>
#include <string> 
#include "sensor/gps/gps_l76k.h"


namespace util {
class Logger {
public:
    using Callback = std::function<void()>;

    struct LogData {
        sensor::GNRMC gnrmc{};
        sensor::GNVTG gnvtg{};
        sensor::GNGGA gngga{};
    };

    int log_interval_ms_;
    bool log_on_;
    std::string csv_file_path_;

    explicit Logger(const std::string& config_path);
    ~Logger();

    // コールバック指定版：cb が指定されていればそれを呼ぶ。なければ OnTick() を呼ぶ
    void Start(std::chrono::milliseconds period, Callback cb);

    void Stop();

    // 周期処理
    void OnTick();

    void WriteCsv(const LogData &log_data);

private:
    void WriteLogHeader();
    std::string GenerateCsvFilePath();
    std::thread th_;
    std::atomic<bool> running_{false};
    std::chrono::milliseconds period_{0};
    Callback cb_{};
};
} // namespace util

#endif // LOGGER_H