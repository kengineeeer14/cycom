#ifndef CYCOM_SRC_APPLICATION_UTIL_LOGGER_H_
#define CYCOM_SRC_APPLICATION_UTIL_LOGGER_H_

#include "domain/sensor/gps_l76k.h"

#include <atomic>
#include <string>
#include <thread>

namespace application::util {
class Logger {
  public:
    struct LogData {
        domain::sensor::GNRMC gnrmc{};
        domain::sensor::GNVTG gnvtg{};
        domain::sensor::GNGGA gngga{};
    };

    /**
     * @brief Loggerを初期化し、ロギングスレッドを自動起動する（Touch クラスと同じパターン）
     *
     * @param config_path 設定ファイルのパス
     * @param gps GPS データソースへの参照
     */
    Logger(const std::string &config_path, domain::sensor::L76k &gps);

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

    domain::sensor::L76k &gps_;
    int log_interval_ms_;
    bool log_on_;
    std::string csv_file_path_;

    std::thread th_;
    std::atomic<bool> running_{false};
};
}  // namespace application::util

#endif  // CYCOM_SRC_APPLICATION_UTIL_LOGGER_H_