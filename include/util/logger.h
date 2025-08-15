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
#include "sensor/uart/gps/gps_l76k.h"


namespace util {
class Logger {
public:
    using Callback = std::function<void()>;

    struct LogData {
        sensor_uart::L76k::GNRMC gnrmc;
        sensor_uart::L76k::GNVTG gnvtg;
        sensor_uart::L76k::GNGGA gngga;
    };

    int log_interval_ms_;
    std::string csv_file_path_;

    explicit Logger(const std::string& config_path);
    ~Logger();

    // コールバック指定版：cb が指定されていればそれを呼ぶ。なければ OnTick() を呼ぶ
    void Start(std::chrono::milliseconds period, Callback cb);

    // メンバ関数 OnTick() を呼ぶシンプル版
    void Start(std::chrono::milliseconds period);

    void Stop();

    // 周期処理
    void OnTick();

    void WriteCsv(const LogData &log_data);

private:
    std::thread th_;
    std::atomic<bool> running_{false};
    std::chrono::milliseconds period_{0};
    Callback cb_{};
};
} // namespace util

#endif // LOGGER_H