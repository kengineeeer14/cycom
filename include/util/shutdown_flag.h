#ifndef UTIL_SHUTDOWN_FLAG_H
#define UTIL_SHUTDOWN_FLAG_H

#include <atomic>

namespace util {

// 全スレッドで共有する終了フラグ
// SIGINT (Ctrl+C) や SIGTERM でtrueに設定される
extern std::atomic<bool> g_shutdown_requested;

} // namespace util

#endif // UTIL_SHUTDOWN_FLAG_H
