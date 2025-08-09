#include <gpiod.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string>

#define CHIP_NAME "/dev/gpiochip0"
#define LINE_FORCE 15   // FORCE_ONピン(GPIO15)
#define LINE_STANDBY 14 // STANDBYピン(GPIO14)
#define UART_PORT "/dev/ttys0"
#define BAUDRATE B9600

bool setup_gpio() {
    gpiod_chip *chip = gpiod_chip_open(CHIP_NAME);
    
    if (!chip) {
        std::cerr << "Failed to open gpiochip\n";
        return false;
    }

    gpiod_line *force_line = gpiod_chip_get_line(chip, LINE_FORCE);
    gpiod_line *standby_line = gpiod_chip_get_line(chip, LINE_STANDBY);

    if (!force_line || !standby_line) {
        std::cerr << "Failed to get GPIO line\n";
        gpiod_chip_close(chip);
        return false;
    }

    // FORCE_ONをHIGH、STANDBYをLOWでGPS起動
    if (gpiod_line_request_output(force_line, "gps_control", 1) < 0 ||
        gpiod_line_request_output(standby_line, "gps_control", 0) < 0) {
        if (gpiod_line_request_output(force_line, "gps_control_force", 1) < 0) {
            std::cerr << "Failed to request FORCE_ON GPIO output\n";
            gpiod_chip_close(chip);
            return false;
        }
        if (gpiod_line_request_output(standby_line, "gps_control_standby", 0) < 0) {
            std::cerr << "Failed to request STANDBY GPIO output\n";
            gpiod_chip_close(chip);
            return false;
        }
        std::cerr << "Failed to request GPIO output\n";
        gpiod_chip_close(chip);
        return false;
    }

    gpiod_chip_close(chip);
    return true;
}

int setup_uart(const char *port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) {
        std::cerr << "Failed to open UART\n";
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, BAUDRATE);
    cfsetospeed(&options, BAUDRATE);

    options.c_cflag |= (CLOCAL | CREAD); // 受信有効
    options.c_cflag &= ~PARENB;          // パリティなし
    options.c_cflag &= ~CSTOPB;          // ストップビット1
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;              // データビット8
    options.c_lflag = 0;                 // 非カノニカルモード
    options.c_oflag = 0;
    options.c_iflag = 0;

    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

int main() {
    if (!setup_gpio()) {
        return 1;
    }

    int fd = setup_uart(UART_PORT);
    if (fd < 0) {
        return 1;
    }

    char buf[256];
    while (true) {
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            std::cout << buf; // NMEA文をそのまま出力
        }
    }

    close(fd);
    return 0;
}