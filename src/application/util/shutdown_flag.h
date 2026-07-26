#ifndef CYCOM_SRC_APPLICATION_UTIL_SHUTDOWN_FLAG_H_
#define CYCOM_SRC_APPLICATION_UTIL_SHUTDOWN_FLAG_H_

#include <atomic>

namespace util {

// 全スレッドで共有する終了フラグ
// SIGINT (Ctrl+C) や SIGTERM でtrueに設定される
extern std::atomic<bool> g_shutdown_requested;

}  // namespace util

#endif  // CYCOM_SRC_APPLICATION_UTIL_SHUTDOWN_FLAG_H_
