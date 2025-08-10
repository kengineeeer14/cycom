#include "gpio/gpio_config.h"

namespace gpio{
    bool GpioConfigure::SetupGpio(){
        gpiod_chip *chip = gpiod_chip_open(chip_name_.c_str());
        
        if (!chip) {
            std::cerr << "Failed to open gpiochip\n";
            return false;
        }

        gpiod_line *force_line = gpiod_chip_get_line(chip, line_force_);
        gpiod_line *standby_line = gpiod_chip_get_line(chip, line_standby_);

        if (!force_line || !standby_line) {
            std::cerr << "Failed to get GPIO line\n";
            gpiod_chip_close(chip);
            return false;
        }

        gpiod_chip_close(chip);
        return true;
    }
}   // namespace gpio