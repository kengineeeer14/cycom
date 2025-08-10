#include <iostream>
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

#ifndef UART_CONFIG_H
#define UART_CONFIG_H

namespace sensor_uart{
    class UartConfigure{
        public:
            int SetupUart();

        private:
            // TODO 定数はjsonで設定できるようにする．
            const std::string uart_port_;
            const unsigned int baudrate_;
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
}   // namespace sensor_uart

#endif // UART_CONFIG_H