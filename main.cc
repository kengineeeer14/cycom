#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include "gpio/gpio_config.h"
#include "sensors/uart/uart_config.h"

int main() {
    const std::string config_path = "config/config.json";
    gpio::GpioConfigure gpio_config(config_path);
    sensor_uart::UartConfigure uart_config(config_path);

    if (!gpio_config.SetupGpio()) {
        return 1;
    }

    int fd = uart_config.SetupUart();
    if (fd < 0) {
        return 1;
    }

    char buf[256];
    while (true) {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            std::cout << buf;
        }
    }

    close(fd);
    return 0;
}
