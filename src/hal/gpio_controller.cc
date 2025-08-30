#include <gpiod.h>
#include <iostream>
#include "hal/gpio_controller.h"


namespace hal {
bool GpioController::SetupGpio() {
    gpiod_chip* chip = gpiod_chip_open(chip_name_.c_str());
    if (!chip) {
        std::cerr << "Failed to open gpiochip: " << chip_name_ << "\n";
        return false;
    }

    gpiod_line* force_line   = gpiod_chip_get_line(chip, line_uart_rx_);
    gpiod_line* standby_line = gpiod_chip_get_line(chip, line_uart_tx_);

    if (!force_line || !standby_line) {
        std::cerr << "Failed to get GPIO line (rx=" << line_uart_rx_
                  << ", tx=" << line_uart_tx_ << ")\n";
        gpiod_chip_close(chip);
        return false;
    }

    gpiod_chip_close(chip);
    return true;
}
}  // namespace hal