#ifndef DEV_CONFIG_H
#define DEV_CONFIG_H

#include <termio.h>
#include <iostream>
#include <wiringPi.h>
#include <wiringSerial.h>
#include <cstdint>

using UBYTE = uint8_t;
using UWORD = uint16_t;
using UDOUBLE = uint32_t;

class UartConfig {
    public:
        /**
         * GPIO config
         */
        static constexpr int DEV_FORCE = 15;
        static constexpr int DEV_STANDBY = 14;

        /**
         * GPIO read and write
        **/
        /**
         * @brief 指定したピンにvalueが0では0Vを出力し，0以外では3.3Vの値を出力する．
         * 
         * @param pin 
         * @param value 
         */
        void DevDigitalWrite(const int &pin, const int &value);

        /**
         * @brief 指定されたGPIOピンの値を読み取り，ピンにかかっている論理レベルに応じて，HIGH(1)またはLOW(0)のいずれかが返される．
         * 
         * @param pin 
         * @return int 
         */
        int DevDigitalRead(const int &pin);

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
        void DevUartSendString(const std::string &data);

        /**
         * @brief 文字列を最大Numとしてdataに受け取り返す．
         * 
         * @param[in] Num 
         * @param[inout] data 
         */
        void DevUartReceiveString(const UWORD Num, std::string &data);

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

        /**
         * @brief 指定したGPIOピンの入出力モードを設定する．1ならINPUT，それ以外はOUTPUT．
         * 
         * @param[in] pin 
         * @param[in] mode 
         */
        void DevSetGpioMode(const UWORD &pin, const UWORD &mode);

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
        static int fd{-1};
        const std::string uart_port{"/dev/ttyS0"};
        

};

#endif // DEV_CONFIG_H