#include "gpio/gpio_config.h"

namespace gpio{
    GpioConfigure::GpioConfigure(const std::string& config_path) {
        // JSONファイルを開く
        std::ifstream ifs(config_path);
        if (!ifs.is_open()) {
            throw std::runtime_error("Failed to open config file");
        }
        nlohmann::json j;
        ifs >> j;

        chip_name_ = j["gpio"]["chip_name"].get<std::string>();
        line_uart_rx_ = j["gpio"]["line_uart_rx"].get<unsigned int>();
        line_uart_tx_ = j["gpio"]["line_uart_tx"].get<unsigned int>();
    }

    bool GpioConfigure::SetupGpio(){
        gpiod_chip *chip = gpiod_chip_open(chip_name_.c_str());
        
        if (!chip) {
            std::cerr << "Failed to open gpiochip\n";
            return false;
        }

        gpiod_line *force_line = gpiod_chip_get_line(chip, line_uart_rx_);
        gpiod_line *standby_line = gpiod_chip_get_line(chip, line_uart_tx_);

        if (!force_line || !standby_line) {
            std::cerr << "Failed to get GPIO line\n";
            gpiod_chip_close(chip);
            return false;
        }

        gpiod_chip_close(chip);
        return true;
    }
}   // namespace gpio