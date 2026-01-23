#include "hal/impl/uart_impl.h"

#include <fcntl.h>
#include <stdexcept>
#include <termios.h>
#include <unistd.h>

namespace hal {

// ボーレート変換ヘルパー
static speed_t ConvertBaudrate(unsigned int baudrate) {
    switch (baudrate) {
        case 0:
            return B0;
        case 50:
            return B50;
        case 75:
            return B75;
        case 110:
            return B110;
        case 134:
            return B134;
        case 150:
            return B150;
        case 200:
            return B200;
        case 300:
            return B300;
        case 600:
            return B600;
        case 1200:
            return B1200;
        case 1800:
            return B1800;
        case 2400:
            return B2400;
        case 4800:
            return B4800;
        case 9600:
            return B9600;
        case 19200:
            return B19200;
        case 38400:
            return B38400;
        case 57600:
            return B57600;
        case 115200:
            return B115200;
        default:
            throw std::invalid_argument("Unsupported baudrate");
    }
}

UartImpl::UartImpl(const std::string& port, unsigned int baudrate) : fd_(-1) {
    fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
    if (fd_ == -1) {
        throw std::runtime_error("Failed to open UART device");
    }

    struct termios options;
    if (tcgetattr(fd_, &options) < 0) {
        ::close(fd_);
        throw std::runtime_error("Failed to get UART attributes");
    }

    speed_t speed = ConvertBaudrate(baudrate);
    cfsetispeed(&options, speed);
    cfsetospeed(&options, speed);

    options.c_cflag |= (CLOCAL | CREAD);  // 受信有効
    options.c_cflag &= ~PARENB;           // パリティなし
    options.c_cflag &= ~CSTOPB;           // ストップビット1
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;  // データビット8
    options.c_lflag = 0;     // 非カノニカルモード
    options.c_oflag = 0;
    options.c_iflag = 0;

    if (tcsetattr(fd_, TCSANOW, &options) < 0) {
        ::close(fd_);
        throw std::runtime_error("Failed to set UART attributes");
    }
}

int UartImpl::GetFileDescriptor() {
    return fd_;
}

ssize_t UartImpl::Read(uint8_t* buffer, size_t len) {
    return ::read(fd_, buffer, len);
}

ssize_t UartImpl::Write(const uint8_t* data, size_t len) {
    return ::write(fd_, data, len);
}

bool UartImpl::IsOpen() {
    return fd_ >= 0;
}

UartImpl::~UartImpl() {
    if (fd_ >= 0) {
        ::close(fd_);
    }
}

}  // namespace hal
