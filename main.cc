#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <iostream>
#include <string>
#include <csignal>
#include "sensors/uart/L76X.h"

UartConfig uartconfig;
L76X l76k;

void Handler(int signo) {
    uartconfig.DevModuleExit();
    std::cout << "\nProgram terminated.\n";
    std::exit(0);
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

int main(int argc, char** argv)
{

    GNRMC GPS;
    Coordinates Baidu;
    char* buffer;

    if (uartconfig.DevModuleInit() == 1) return 1;

    // 割り込み処理：Ctrl+Cが送られると，Handler関数が呼び出され，終了処理される．
    std::signal(SIGINT, Handler);

    uartconfig.DevDelayMs(100);
    uartconfig.DevSetBaudrate(9600);
    uartconfig.DevDelayMs(100);

    while (true) {
        GPS = l76k.GetGNRMC();
        std::cout << "\r\n";
        std::cout << "Time: " << static_cast<int>(GPS.Time_H) << ":"
                  << static_cast<int>(GPS.Time_M) << ":"
                  << static_cast<int>(GPS.Time_S) << "\r\n";
        std::cout << "Latitude and longitude: " << GPS.Lat << " " << GPS.Lat_area << " "
                  << GPS.Lon << " " << GPS.Lon_area << "\r\n";

        Baidu = L76X_Baidu_Coordinates();
        std::cout << "Baidu Coordinates: " << Baidu.Lat << ", " << Baidu.Lon << "\r\n";
    }

    uartconfig.DevModuleExit();
    return 0;
}
