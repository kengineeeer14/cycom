#ifndef DEV_CONFIG_H
#define DEV_CONFIG_H

#include <gpiod.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdint>
#include <string>

using UBYTE = uint8_t;
using UWORD = uint16_t;
using UDOUBLE = uint32_t;

class UartConfig {
    public:
        /**
         * GPIO config
         */
        static constexpr unsigned int DEV_FORCE = 15;
        static constexpr unsigned int DEV_STANDBY = 14;

        /**
         * GPIO read and write
        **/
        /**
         * @brief 指定したピンにvalueが0では0Vを出力し，0以外では3.3Vの値を出力する．
         * 
         * @param pin 
         * @param value 
         */
        void DevDigitalWrite(const unsigned int &pin, const int &value);

        /**
         * @brief 指定されたGPIOピンの値を読み取り，ピンにかかっている論理レベルに応じて，HIGH(1)またはLOW(0)のいずれかが返される．
         * 
         * @param pin 
         * @return int 
         */
        int DevDigitalRead(const unsigned int &pin);

        /**
         * @brief 指定したミリ秒数だけ待機する．
         * 
         * @param[in] xms 
         */
        void DevDelayMs(const unsigned int &xms);

        /******************************************************************************
        function:	
            Uart receiving and sending
        ******************************************************************************/

        /**
         * @brief シリアルデバイスから1文字を取得する．
         * 
         * @return UBYTE 
         */
        UBYTE DevUartReceiveByte();

        /**
         * @brief 1文字をシリアルポートに送信する．
         * 
         * @param[in] data 
         */
        void DevUartSendByte(const char &data);

        /**
         * @brief 文字列を1文字ずつシリアルポートに送信する．
         * 
         * @param[in] data 
         */
        void DevUartSendString(const char *data);

        /**
         * @brief 文字列を最大Numとしてdataに受け取り返す．
         * 
         * @param[in] Num 
         * @param[inout] data 
         */
        void DevUartReceiveString(const UWORD Num, char *data);

        /******************************************************************************
        function:	
            Set the serial port baud rate
        ******************************************************************************/
        /**
         * @brief 指定された通信ポートに対して，指定されたボーレートで接続を再設定する．
         * 
         * @param[in] Baudrate 
         */
        void DevSetBaudrate(const UDOUBLE &Baudrate);

        /******************************************************************************
        function:	
            Module initialization, BCM2835 library and initialization pins,
            uart
        ******************************************************************************/

        /**
         * @brief UART通信とGPIOの初期化を行う
         * 
         * @return UBYTE 
         */
        UBYTE DevModuleInit();

        /**
         * @brief UART通信とGPIOの終了処理を行う
         * 
         */
        void DevModuleExit();
    
    private:
        int fd{-1};
        const std::string uart_port{"/dev/ttyS0"};
        // libgpiod用のGPIO chipハンドル
        void* chip_{nullptr};  // 実装でgpiod_chip*
        void* line_force_{nullptr};  // 実装でgpiod_line*
        void* line_standby_{nullptr};
        

};

#endif // DEV_CONFIG_H