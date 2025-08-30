#include "hal/gpio_line.h"
#include "util/time_unit.h"

namespace hal {

gpiod_chip* GpioLine::OpenChipFlexible(const std::string& chip) {
    if (chip.rfind("/dev/", 0) == 0) {
        return gpiod_chip_open(chip.c_str());
    }
    return gpiod_chip_open_by_name(chip.c_str());
}

GpioLine::GpioLine(const std::string& chip,const unsigned int &offset,const  bool &output,const int &initial)
: is_output_(output) {
    chip_ = OpenChipFlexible(chip);
    if (!chip_) throw std::runtime_error("gpiod_chip_open failed");

    line_ = gpiod_chip_get_line(chip_, offset);
    if (!line_) throw std::runtime_error("gpiod_chip_get_line failed");

    if (output) {
        if (gpiod_line_request_output(line_, "cycom", initial) < 0) {
            throw std::runtime_error("gpiod_line_request_output failed");
        }
    }
}

void GpioLine::RequestRisingEdge() {
    if (is_output_) throw std::runtime_error("line is output");
    gpiod_line_release(line_);
    if (gpiod_line_request_rising_edge_events(line_, consumer_) < 0) {
        throw std::runtime_error("gpiod_line_request_rising_edge_events failed");
    }
}

void GpioLine::RequestFallingEdge() {
    if (is_output_) throw std::runtime_error("line is output");
    gpiod_line_release(line_);
    if (gpiod_line_request_falling_edge_events(line_, consumer_) < 0) {
        throw std::runtime_error("gpiod_line_request_falling_edge_events failed");
    }
}

int GpioLine::WaitEvent(const int &timeout_ms) {
    timespec ts{static_cast<time_t>(timeout_ms * util::TimeUnit::kMs2Sec),static_cast<long>(util::TimeUnit::msWithinMs(timeout_ms)*util::TimeUnit::kMs2Ns)};
    return gpiod_line_event_wait(line_, &ts);
}

void GpioLine::ReadEvent(gpiod_line_event &ev) {
    if (gpiod_line_event_read(line_, &ev) < 0) {
        throw std::runtime_error("gpiod_line_event_read failed");
    }
}

void GpioLine::Set(const int &value) {
    if (!is_output_) throw std::runtime_error("line is not output");
    if (gpiod_line_set_value(line_, value) < 0) {
        throw std::runtime_error("gpiod_line_set_value failed");
    }
}

GpioLine::~GpioLine() {
    if (line_) gpiod_line_release(line_);
    if (chip_) gpiod_chip_close(chip_);
}

}  // namespace hal