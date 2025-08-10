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
            const std::string chip_name_{"/dev/gpiochip0"};
            const unsigned int line_force_{15U};
            const unsigned int line_standby_{14U};
    };
}   // namespace gpio

#endif // GPIO_CONFIG_H 