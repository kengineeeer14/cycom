#include "application/util/shutdown_flag.h"

namespace application::util {

std::atomic<bool> g_shutdown_requested{false};

}  // namespace application::util
