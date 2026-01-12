#include "util/shutdown_flag.h"

namespace util {

std::atomic<bool> g_shutdown_requested{false};

} // namespace util
