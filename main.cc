#include <iostream>
#include <csignal>
#include "gpio/DEV_Config.h"
#include "sensors/uart/L76X.h"

UartConfig uartconfig;
L76X l76k;

void Handler(int signo)
{
    std::cout << "\r\nHandler: Program stop\r\n";
    uartconfig.DevModuleExit();
    std::exit(0);
}

int main() {
    std::string uart_port{"/dev/ttyS0"};
    int fd = serialOpen(uart_port.c_str(), 9600);
    if (fd < 0) {
        std::cerr << "Unable to open serial device\n";
        return 1;
    }

    while (true) {
        char c = serialGetchar(fd);
        std::cout << c;
    }
    serialClose(fd);
    return 0;
}

// int main(int argc, char** argv)
// {

//     GNRMC GPS;
//     Coordinates Baidu;
//     char* buffer;

//     if (uartconfig.DevModuleInit() == 1) return 1;

//     // 割り込み処理：Ctrl+Cが送られると，Handler関数が呼び出され，終了処理される．
//     std::signal(SIGINT, Handler);

//     uartconfig.DevDelayMs(100);
//     uartconfig.DevSetBaudrate(9600);
//     uartconfig.DevDelayMs(100);

//     while (true) {
//     char* buf = l76k.Test();
//     for (int i = 0; i < 32; ++i) {  // 受信バッファの一部を16進数表示
//         printf("%02X ", (unsigned char)buf[i]);
//     }
//     printf("\n");


//         // GPS = l76k.GetGNRMC();
//         // std::cout << "\r\n";
//         // std::cout << "Time: " << static_cast<int>(GPS.Time_H) << ":"
//         //           << static_cast<int>(GPS.Time_M) << ":"
//         //           << static_cast<int>(GPS.Time_S) << "\r\n";
//         // std::cout << "Latitude and longitude: " << GPS.Lat << " " << GPS.Lat_area << " "
//         //           << GPS.Lon << " " << GPS.Lon_area << "\r\n";

//         // Baidu = L76X_Baidu_Coordinates();
//         // std::cout << "Baidu Coordinates: " << Baidu.Lat << ", " << Baidu.Lon << "\r\n";
//     }

//     uartconfig.DevModuleExit();
//     return 0;
// }
