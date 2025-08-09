#include <gpiod.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string>

#define CHIP_NAME "/dev/gpiochip0"
#define LINE_FORCE 15   // FORCE_ONピン(GPIO15)
#define LINE_STANDBY 14 // STANDBYピン(GPIO14)
#define UART_PORT "/dev/ttyS0"
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

    if (!force_line) {
        std::cerr << "force_line is nullptr\n";
        gpiod_chip_close(chip);
        return false;
    }
    if (!standby_line) {
        std::cerr << "standby_line is nullptr\n";
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
        std::cout << "Waiting for data...\n";
        int n = read(fd, buf, sizeof(buf) - 1);
        if (n > 0) {
            buf[n] = '\0';
            std::cout << "Received: " << buf << std::endl;
        } else if (n == 0) {
            std::cout << "No data received\n";
        } else {
            std::cerr << "Read error\n";
        }
        // usleep(500000);  // 0.5秒待つ
    }

    close(fd);
    return 0;
}

// int main() {
//     std::string uart_port = "/dev/ttyS0";
//     int fd = open(uart_port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
//     if (fd < 0) {
//         std::cerr << "Failed to open " << uart_port << "\n";
//         return 1;
//     }

//     termios tty{};
//     tcgetattr(fd, &tty);
//     cfsetospeed(&tty, B9600);
//     cfsetispeed(&tty, B9600);
//     tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
//     tty.c_cflag |= (CLOCAL | CREAD);
//     tty.c_cflag &= ~(PARENB | PARODD);
//     tty.c_cflag &= ~CSTOPB;
//     tty.c_cflag &= ~CRTSCTS;
//     tty.c_iflag = 0;
//     tty.c_oflag = 0;
//     tty.c_lflag = 0;
//     tty.c_cc[VMIN] = 0;
//     tty.c_cc[VTIME] = 5;
//     tcsetattr(fd, TCSANOW, &tty);

//     char buf[256];
//     while (true) {
//         int n = read(fd, buf, sizeof(buf) - 1);
//         if (n > 0) {
//             buf[n] = '\0';
//             std::cout << buf;
//         }
//     }

//     close(fd);
//     return 0;
// }