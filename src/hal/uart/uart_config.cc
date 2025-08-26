#include "hal/uart/uart_config.h"

namespace sensor_uart{
    UartConfigure::UartConfigure(const std::string &config_path) {
        std::ifstream ifs(config_path);
        if (!ifs.is_open()) {
            throw std::runtime_error("Failed to open config file");
        }
        nlohmann::json j;
        ifs >> j;

        baudrate_ = j["sensor_uart"]["baudrate"].get<unsigned int>();
        ConvertBaudrateToSpeed(baudrate_, baudrate_speed_);
    }

    int UartConfigure::SetupUart(){
        int fd = open(uart_port_.c_str(), O_RDWR | O_NOCTTY | O_SYNC);
        if (fd == -1) {
            std::cerr << "Failed to open UART\n";
            return -1;
        }

        struct termios options;
        tcgetattr(fd, &options);
        cfsetispeed(&options, baudrate_speed_);
        cfsetospeed(&options, baudrate_speed_);

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

    void UartConfigure::ConvertBaudrateToSpeed(const unsigned int &baudrate,
                                                speed_t &baudrate_speed) {
        switch (baudrate) {
            case 0: baudrate_speed = B0; break;
            case 50: baudrate_speed = B50; break;
            case 75: baudrate_speed = B75; break;
            case 110: baudrate_speed = B110; break;
            case 134: baudrate_speed = B134; break;
            case 150: baudrate_speed = B150; break;
            case 200: baudrate_speed = B200; break;
            case 300: baudrate_speed = B300; break;
            case 600: baudrate_speed = B600; break;
            case 1200: baudrate_speed = B1200; break;
            case 1800: baudrate_speed = B1800; break;
            case 2400: baudrate_speed = B2400; break;
            case 4800: baudrate_speed = B4800; break;
            case 9600: baudrate_speed = B9600; break;
            case 19200: baudrate_speed = B19200; break;
            case 38400: baudrate_speed = B38400; break;
            default:
                throw std::invalid_argument("設定したbaudrateはサポートされていません．");
        }
    }
}   // namespace sensor_uart