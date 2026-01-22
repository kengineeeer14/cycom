#ifndef CYCOM_HAL_INTERFACE_I_UART_H_
#define CYCOM_HAL_INTERFACE_I_UART_H_

#include <cstddef>
#include <cstdint>

namespace hal {

/**
 * @brief UART通信の抽象インターフェース
 * 
 * ハードウェアに依存しないUARTシリアル通信のインターフェース。
 * テスト時にはモック実装を、本番環境では実ハードウェア実装を使用する。
 */
class IUart {
public:
    virtual ~IUart() = default;

    /**
     * @brief UARTポートのファイルディスクリプタを取得する
     * 
     * @return int ファイルディスクリプタ（エラー時は負の値）
     */
    virtual int GetFileDescriptor() = 0;

    /**
     * @brief UARTポートからデータを読み取る
     * 
     * @param buffer 読み取ったデータを格納するバッファ
     * @param len 読み取るバイト数
     * @return ssize_t 実際に読み取ったバイト数（エラー時は負の値）
     */
    virtual ssize_t Read(uint8_t* buffer, size_t len) = 0;

    /**
     * @brief UARTポートにデータを書き込む
     * 
     * @param data 書き込むデータの先頭ポインタ
     * @param len 書き込むバイト数
     * @return ssize_t 実際に書き込んだバイト数（エラー時は負の値）
     */
    virtual ssize_t Write(const uint8_t* data, size_t len) = 0;

    /**
     * @brief UARTポートが開いているか確認する
     * 
     * @return true 開いている
     * @return false 閉じている
     */
    virtual bool IsOpen() = 0;
};

}  // namespace hal

#endif  // CYCOM_HAL_INTERFACE_I_UART_H_
