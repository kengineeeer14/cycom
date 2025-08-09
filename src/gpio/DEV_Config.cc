#include "gpio/DEV_Config.h"

// libgpiodの型を隠すためにキャスト利用
#define CHIP_PTR reinterpret_cast<gpiod_chip*>(chip_)
#define LINE_FORCE_PTR reinterpret_cast<gpiod_line*>(line_force_)
#define LINE_STANDBY_PTR reinterpret_cast<gpiod_line*>(line_standby_)

void UartConfig::DevDigitalWrite(const unsigned int &pin, const int &value) {
    if (!LINE_STANDBY_PTR) {
        std::cerr << "GPIO line_standby not initialized\n";
        return;
    }
    int v = (value == 0) ? 0 : 1;
    if (gpiod_line_set_value(LINE_STANDBY_PTR, v) < 0) {
        std::cerr << "Failed to set GPIO line value\n";
    }
}

int UartConfig::DevDigitalRead(const unsigned int &pin) {
    if (!LINE_FORCE_PTR) {
        std::cerr << "GPIO line_force not initialized\n";
        return -1;
    }
    int val = gpiod_line_get_value(LINE_FORCE_PTR);
    if (val < 0) {
        std::cerr << "Failed to get GPIO line value\n";
        return -1;
    }
    return val;
}

void UartConfig::DevDelayMs(const unsigned int &xms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(xms));
}


UBYTE UartConfig::DevUartReceiveByte() {
    if (fd < 0) return 0xFF;
    char c;
    int n = read(fd, &c, 1);
    if (n == 1) return static_cast<UBYTE>(c);
    else return 0xFF;
}

void UartConfig::DevUartSendByte(const char &data) {
    if (fd < 0) return;
    write(fd, &data, 1);
}

void UartConfig::DevUartSendString(const char *data) {
    if (fd < 0) return;
    size_t len = strlen(data);
    write(fd, data, len);
}

void UartConfig::DevUartReceiveString(const UWORD Num, char *data) {
    if (fd < 0) {
        if (data && Num > 0) data[0] = '\0';
        return;
    }
    ssize_t read_len = read(fd, data, Num - 1);
    if (read_len < 0) read_len = 0;
    data[read_len] = '\0';
}

void UartConfig::DevSetBaudrate(const UDOUBLE &Baudrate) {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }

    fd = open(uart_port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        std::cerr << "Failed to open serial port " << uart_port << "\n";
        return;
    }

    struct termios tty{};
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Failed to get termios attributes\n";
        close(fd);
        fd = -1;
        return;
    }

    speed_t speed;
    switch (Baudrate) {
        case 9600: speed = B9600; break;
        case 19200: speed = B19200; break;
        case 38400: speed = B38400; break;
        case 115200: speed = B115200; break;
        default:
            std::cerr << "Unsupported baud rate. Using 9600.\n";
            speed = B9600;
    }

    cfsetospeed(&tty, speed);
    cfsetispeed(&tty, speed);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;  // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                      // disable break processing
    tty.c_lflag = 0;                             // no signaling chars, no echo, no canonical processing
    tty.c_oflag = 0;                             // no remapping, no delays
    tty.c_cc[VMIN] = 1;                          // read blocks until 1 char arrives
    tty.c_cc[VTIME] = 5;                         // 0.5 seconds read timeout

    tty.c_cflag |= (CLOCAL | CREAD);             // ignore modem controls, enable reading
    tty.c_cflag &= ~(PARENB | PARODD);           // no parity
    tty.c_cflag &= ~CSTOPB;                      // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                     // no hardware flow control

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Failed to set termios attributes\n";
        close(fd);
        fd = -1;
        return;
    }

    std::cout << "Serial port " << uart_port << " opened at " << Baudrate << " baud.\n";
}

UBYTE UartConfig::DevModuleInit() {
    chip_ = gpiod_chip_open_by_name("gpiochip0");
    if (!CHIP_PTR) {
        std::cerr << "Failed to open gpiochip0\n";
        return 1;
    }

    line_force_ = gpiod_chip_get_line(CHIP_PTR, DEV_FORCE);
    line_standby_ = gpiod_chip_get_line(CHIP_PTR, DEV_STANDBY);

    if (!LINE_FORCE_PTR || !LINE_STANDBY_PTR) {
        std::cerr << "Failed to get GPIO lines\n";
        if (CHIP_PTR) gpiod_chip_close(CHIP_PTR);
        return 1;
    }

    // line_force を INPUT モードで要求
    if (gpiod_line_request_input(LINE_FORCE_PTR, "cycom_force") < 0) {
        std::cerr << "Failed to request line_force as input\n";
        gpiod_chip_close(CHIP_PTR);
        return 1;
    }

    // line_standby を OUTPUT モードで要求し初期値 LOW
    if (gpiod_line_request_output(LINE_STANDBY_PTR, "cycom_standby", 0) < 0) {
        std::cerr << "Failed to request line_standby as output\n";
        gpiod_line_release(LINE_FORCE_PTR);
        gpiod_chip_close(CHIP_PTR);
        return 1;
    }

    // シリアルポート開く
    fd = open(uart_port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) {
        std::cerr << "Failed to open serial port " << uart_port << "\n";
        gpiod_line_release(LINE_FORCE_PTR);
        gpiod_line_release(LINE_STANDBY_PTR);
        gpiod_chip_close(CHIP_PTR);
        return 1;
    }

    // 9600bps設定（初期）
    DevSetBaudrate(9600);

    std::cout << "Module init success\n";
    return 0;
}

void UartConfig::DevModuleExit() {
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
    if (LINE_FORCE_PTR) gpiod_line_release(LINE_FORCE_PTR);
    if (LINE_STANDBY_PTR) gpiod_line_release(LINE_STANDBY_PTR);
    if (CHIP_PTR) gpiod_chip_close(CHIP_PTR);
}