#ifndef LOGGER_H
#define LOGGER_H

#include <atomic>
#include <thread>
#include <string> 
#include "sensor/gps/gps_l76k.h"


namespace util {
class Logger {
public:
    struct LogData {
        sensor::GNRMC gnrmc{};
        sensor::GNVTG gnvtg{};
        sensor::GNGGA gngga{};
    };

    /**
     * @brief Loggerを初期化し、ロギングスレッドを自動起動する（Touch クラスと同じパターン）
     * 
     * @param config_path 設定ファイルのパス
     * @param gps GPS データソースへの参照
     */
    Logger(const std::string& config_path, sensor::L76k& gps);
    
    /**
     * @brief ロギングスレッドを安全に停止させる
     */
    ~Logger();

private:
    void WriteLogHeader();
    void WriteCsv(const LogData &log_data);
    std::string GenerateCsvFilePath();
    
    // Touch クラスと同様、内部でスレッドを管理
    void Start();
    void Stop();
    void LoggingLoop();

    sensor::L76k& gps_;
    int log_interval_ms_;
    bool log_on_;
    std::string csv_file_path_;
    
    std::thread th_;
    std::atomic<bool> running_{false};
};
} // namespace util

#endif // LOGGER_H