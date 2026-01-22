#ifndef CYCOM_HAL_INTERFACE_I_SPI_H_
#define CYCOM_HAL_INTERFACE_I_SPI_H_

#include <cstdint>
#include <cstddef>

namespace hal {

/**
 * @brief SPI通信の抽象インターフェース
 * 
 * ハードウェアに依存しないSPIバス通信のインターフェース。
 * テスト時にはモック実装を、本番環境では実ハードウェア実装を使用する。
 */
class ISpi {
public:
    virtual ~ISpi() = default;

    /**
     * @brief SPIバスにデータを送信する
     * 
     * @param data 送信するデータの先頭ポインタ
     * @param len 送信するバイト数
     */
    virtual void WriteBytes(const uint8_t* data, size_t len) = 0;

    /**
     * @brief SPIバスからデータを読み取る
     * 
     * @param buffer 読み取ったデータを格納するバッファ
     * @param len 読み取るバイト数
     */
    virtual void ReadBytes(uint8_t* buffer, size_t len) = 0;

    /**
     * @brief SPIバスで同時に送受信を行う（全二重通信）
     * 
     * @param tx_data 送信するデータの先頭ポインタ
     * @param rx_buffer 受信データを格納するバッファ
     * @param len 送受信するバイト数
     */
    virtual void Transfer(const uint8_t* tx_data, uint8_t* rx_buffer, size_t len) = 0;
};

}  // namespace hal

#endif  // CYCOM_HAL_INTERFACE_I_SPI_H_
