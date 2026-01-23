#include "hal/impl/gpio_impl.h"

#include "util/time_unit.h"

namespace hal {

gpiod_chip* GpioImpl::OpenChipFlexible(const std::string& chip) {
    if (chip.rfind("/dev/", 0) == 0) {
        return gpiod_chip_open(chip.c_str());
    }
    return gpiod_chip_open_by_name(chip.c_str());
}

GpioImpl::GpioImpl(const std::string& chip, unsigned int offset, bool output, int initial)
    : is_output_(output) {
    chip_ = OpenChipFlexible(chip);
    if (!chip_)
        throw std::runtime_error("gpiod_chip_open failed");

    line_ = gpiod_chip_get_line(chip_, offset);
    if (!line_)
        throw std::runtime_error("gpiod_chip_get_line failed");

    if (output) {
        if (gpiod_line_request_output(line_, consumer_, initial) < 0) {
            throw std::runtime_error("gpiod_line_request_output failed");
        }
    }
}

void GpioImpl::Set(int value) {
    if (!is_output_)
        throw std::runtime_error("line is not output");
    if (gpiod_line_set_value(line_, value) < 0) {
        throw std::runtime_error("gpiod_line_set_value failed");
    }
}

int GpioImpl::Get() {
    int value = gpiod_line_get_value(line_);
    if (value < 0) {
        throw std::runtime_error("gpiod_line_get_value failed");
    }
    return value;
}

void GpioImpl::RequestRisingEdge() {
    if (is_output_)
        throw std::runtime_error("line is output");
    gpiod_line_release(line_);
    if (gpiod_line_request_rising_edge_events(line_, consumer_) < 0) {
        throw std::runtime_error("gpiod_line_request_rising_edge_events failed");
    }
}

void GpioImpl::RequestFallingEdge() {
    if (is_output_)
        throw std::runtime_error("line is output");
    gpiod_line_release(line_);
    if (gpiod_line_request_falling_edge_events(line_, consumer_) < 0) {
        throw std::runtime_error("gpiod_line_request_falling_edge_events failed");
    }
}

bool GpioImpl::WaitForEvent(int timeout_sec) {
    timespec ts{static_cast<time_t>(timeout_sec), 0};
    int result = gpiod_line_event_wait(line_, &ts);
    if (result < 0) {
        throw std::runtime_error("gpiod_line_event_wait failed");
    }
    return result > 0;
}

void GpioImpl::ReadEvent(gpiod_line_event& ev) {
    if (gpiod_line_event_read(line_, &ev) < 0) {
        throw std::runtime_error("gpiod_line_event_read failed");
    }
}

GpioImpl::~GpioImpl() {
    if (line_)
        gpiod_line_release(line_);
    if (chip_)
        gpiod_chip_close(chip_);
}

}  // namespace hal
