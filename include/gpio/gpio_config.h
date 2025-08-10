#include <iostream>
#include <gpiod.h>

#ifndef GPIO_CONFIG_H
#define GPIO_CONFIG_H
 
namespace gpio {
    class GpioConfigure{
        public:
            bool SetupGpio();

        private:
            // TODO 定数はjsonで設定できるようにする．
            const std::string chip_name_;
            const unsigned int line_uart_rx_;
            const unsigned int line_uart_tx_;
    };
}   // namespace gpio

#endif // GPIO_CONFIG_H 