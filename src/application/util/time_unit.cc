#include "application/util/time_unit.h"

namespace util {

int TimeUnit::msWithinMs(const int &time_ms) {
    return time_ms % 1000;
}

}  // namespace util