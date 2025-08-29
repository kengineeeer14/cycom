#ifndef GPIO_CONTROLLER_H
#define GPIO_CONTROLLER_H

#include <gpiod.h>
#include <string>
#include <iostream>

namespace gpio {

class GpioController {
public:
    bool SetupGpio();

private:
    const std::string chip_name_{"/dev/gpiochip0"};
    const unsigned int line_uart_rx_{15};  // RX: 入力
    const unsigned int line_uart_tx_{14};  // TX: 出力
};

}  // namespace gpio

#endif  // GPIO_CONTROLLER_H