#ifndef CYCOM_SRC_HAL_UART_IMPL_H_
#define CYCOM_SRC_HAL_UART_IMPL_H_

#include "hal/interface/i_uart.h"

#include <cstddef>
#include <cstdint>
#include <string>

namespace hal {

/**
 * @brief UART通信の実装クラス（Linux termiosデバイス使用）
 *
 * IUartインターフェースを実装し、Raspberry PiのUARTポートを制御します。
 * /dev/ttyS*デバイスファイルを使用した実ハードウェア実装です。
 */
class UartImpl : public IUart {
  public:
    /**
     * @brief UARTデバイスを初期化する
     *
     * @param port UARTデバイスファイルのパス（例: "/dev/ttyS0"）
     * @param baudrate ボーレート（例: 9600, 115200）
     */
    UartImpl(const std::string &port, unsigned int baudrate);

    ~UartImpl() override;

    // IUartインターフェースの実装
    int GetFileDescriptor() override;
    ssize_t Read(uint8_t *buffer, size_t len) override;
    ssize_t Write(const uint8_t *data, size_t len) override;
    bool IsOpen() override;

  private:
    int fd_;
};

}  // namespace hal

#endif  // CYCOM_SRC_HAL_UART_IMPL_H_
