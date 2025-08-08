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

int main(int argc, char** argv)
{

    GNRMC GPS;
    Coordinates Baidu;

    if (uartconfig.DevModuleInit() == 1) return 1;

    // 割り込み処理：Ctrl+Cが送られると，Handler関数が呼び出され，終了処理される．
    std::signal(SIGINT, Handler);

    uartconfig.DevDelayMs(100);
    uartconfig.DevSetBaudrate(9600);
    uartconfig.DevDelayMs(100);

    while (true) {
        GPS = l76k.Test();
        std::cout << "\r\n";
        std::cout << "Lon: " << GPS.Lon << "\n";
        std::cout << "Lat: " << GPS.Lat << "\n";
        std::cout << "Lon_area: " << GPS.Lon_area << "\n";
        std::cout << "Lat_area: " << GPS.Lat_area << "\n";
        std::cout << "Time_H: " << GPS.LTime_Hon << "\n";
        std::cout << "Time_M: " << GPS.Time_M << "\n";
        std::cout << "Time_S: " << GPS.Time_S << "\n";
        std::cout << "Status: " << GPS.Status << "\n";


        // GPS = l76k.GetGNRMC();
        // std::cout << "\r\n";
        // std::cout << "Time: " << static_cast<int>(GPS.Time_H) << ":"
        //           << static_cast<int>(GPS.Time_M) << ":"
        //           << static_cast<int>(GPS.Time_S) << "\r\n";
        // std::cout << "Latitude and longitude: " << GPS.Lat << " " << GPS.Lat_area << " "
        //           << GPS.Lon << " " << GPS.Lon_area << "\r\n";

        // Baidu = L76X_Baidu_Coordinates();
        // std::cout << "Baidu Coordinates: " << Baidu.Lat << ", " << Baidu.Lon << "\r\n";
    }

    uartconfig.DevModuleExit();
    return 0;
}
