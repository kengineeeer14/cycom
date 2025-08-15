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

namespace util {
class Logger {
public:
    using Callback = std::function<void()>;

    int log_interval_ms_;

    explicit Logger(const std::string& config_path);
    ~Logger();

    // コールバック指定版：cb が指定されていればそれを呼ぶ。なければ OnTick() を呼ぶ
    void Start(std::chrono::milliseconds period, Callback cb);

    // メンバ関数 OnTick() を呼ぶシンプル版
    void Start(std::chrono::milliseconds period);

    void Stop();

    // 周期処理（必要に応じて中身を編集）
    void OnTick();

private:
    std::thread th_;
    std::atomic<bool> running_{false};
    std::chrono::milliseconds period_{0};
    Callback cb_{};
};
} // namespace util

#endif // LOGGER_H