#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <fstream>          // for std::ifstream
#include <nlohmann/json.hpp> // for JSON handling

#ifndef UART_CONFIG_H
#define UART_CONFIG_H

namespace hal{
    class UartConfigure{
        public:
            explicit UartConfigure(const std::string& config_path);
            int SetupUart();

        private:
            const std::string uart_port_{"/dev/ttyS0"};
            unsigned int baudrate_;
            speed_t baudrate_speed_;

            /**
             * @brief int型のボーレートをtermios用に変換する．
             * 
             * @param[in] baudrate 
             * return speed_t
             */
            void ConvertBaudrateToSpeed(const unsigned int &baudrate,
                                        speed_t &baudrate_speed);
    };
}   // namespace hal

#endif // UART_CONFIG_H