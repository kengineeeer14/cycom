#include <iostream>
#include <csignal>
#include "gpio/DEV_Config.h"
#include "L76X.h"

void Handler(int signo)
{
    std::cout << "\r\nHandler: Program stop\r\n";
    DevModuleExit();
    std::exit(0);
}

int main(int argc, char** argv)
{
    GNRMC GPS;
    Coordinates Baidu;

    if (DevModuleInit() == 1) return 1;

    // 割り込み処理：Ctrl+Cが送られると，Handler関数が呼び出され，終了処理される．
    std::signal(SIGINT, Handler);

    DEV_Delay_ms(100);
    DevSetBaudrate(9600);
    DEV_Delay_ms(100);

    while (true) {
        GPS = L76X_Gat_GNRMC();
        std::cout << "\r\n";
        std::cout << "Time: " << static_cast<int>(GPS.Time_H) << ":"
                  << static_cast<int>(GPS.Time_M) << ":"
                  << static_cast<int>(GPS.Time_S) << "\r\n";
        std::cout << "Latitude and longitude: " << GPS.Lat << " " << GPS.Lat_area << " "
                  << GPS.Lon << " " << GPS.Lon_area << "\r\n";

        // Baidu = L76X_Baidu_Coordinates();
        // std::cout << "Baidu Coordinates: " << Baidu.Lat << ", " << Baidu.Lon << "\r\n";
    }

    DevModuleExit();
    return 0;
}
