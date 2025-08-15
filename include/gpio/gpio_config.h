#include <iostream>
#include <gpiod.h>

#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H
 
namespace gpio {
    class GpioConfigure{
        public:
            bool SetupGpio();

        private:
            const std::string chip_name_{"/dev/gpiochip0"};
            const unsigned int line_uart_rx_{15};
            const unsigned int line_uart_tx_{14};
    };
}   // namespace gpio

#endif // GPIO_CONFIG_H 