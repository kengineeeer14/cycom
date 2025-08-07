#include "gpio/DEV_Config.h"

void UartConfig::DevDigitalWrite(const int &pin, const int &value) {
    digitalWrite(pin, value == 0 ? LOW : HIGH);
}

int UartConfig::DevDigitalRead(const int &pin) {
    return digitalRead(pin);
}

void UartConfig::DevDelayMs(const unsigned int &xms) {
    delay(xms);
}

UBYTE UartConfig::DevUartReceiveByte() {
    return serialGetchar(fd);
}

void UartConfig::DevUartSendByte(const char &data) {
    serialPutchar(fd, data);
}

void UartConfig::DevUartSendString(const std::string &data)
{
    for (char c : data) {
        serialPutchar(fd, c);
    }
}

void UartConfig::DevUartReceiveString(const UWORD Num, std::string &data)
{
    data.clear();
    for (UWORD i = 0; i < Num - 1; ++i) {
        data.push_back(serialGetchar(fd));
    }
    data.push_back('\0');
}

void UartConfig::DevSetBaudrate(const UDOUBLE &Baudrate) {
    serialClose(fd);
    fd = serialOpen(uart_port, Baudrate);
    if (fd < 0) {
        std::cerr << "set uart failed !!!" << std::endl;
    } else {
        std::cout << "set uart success !!!" << std::endl;
    }
}

void UartConfig::DevSetGpioMode(const UWORD &pin, const UWORD &mode) {
    pinMode(pin, (mode == 1) ? INPUT : OUTPUT);
}

UBYTE UartConfig::DevModuleInit() {
    if (wiringPiSetupGpio() < 0) {
        std::cerr << "set wiringPi lib failed !!!" << std::endl;
        return 1;
    } else {
        std::cout << "set wiringPi lib success !!!" << std::endl;
    }

    fd = serialOpen(uart_port, 9600);
    if (fd < 0) {
        return 1;
    } else {
        std::cout << "set uart success !!!" << std::endl;
    }

    pinMode(DEV_FORCE, INPUT);
    pinMode(DEV_STANDBY, OUTPUT);
    DevDigitalWrite(DEV_STANDBY, 0);
    return 0;
}

void UartConfig::DevModuleExit() {
    serialFlush(fd);
    serialClose(fd);
    pinMode(DEV_FORCE, INPUT);
    pinMode(DEV_STANDBY, INPUT);
}