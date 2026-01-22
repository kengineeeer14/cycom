#ifndef CYCOM_HAL_IMPL_SPI_IMPL_H_
#define CYCOM_HAL_IMPL_SPI_IMPL_H_

#include <cstdint>
#include <cstddef>
#include "hal/interface/i_spi.h"

namespace hal {

/**
 * @brief SPI通信の実装クラス（Linux SPIデバイス使用）
 * 
 * ISpiインターフェースを実装し、Raspberry PiのSPIバスを制御します。
 * /dev/spide vデバイスファイルを使用した実ハードウェア実装です。
 */
class SpiImpl : public ISpi {
public:
    /**
     * @brief SPIデバイスを初期化する
     * 
     * @param dev SPIデバイスファイルのパス（例: "/dev/spidev0.0"）
     * @param speed_hz 通信クロック周波数[Hz]
     * @param mode SPIモード（0〜3）
     * @param bits 1ワードあたりのビット数
     */
    explicit SpiImpl(const char* dev,
                     uint32_t speed_hz = 40000000,
                     uint8_t mode = 0,
                     uint8_t bits = 8);

    // コピー禁止、ムーブのみ許可
    SpiImpl(const SpiImpl&) = delete;
    SpiImpl& operator=(const SpiImpl&) = delete;
    SpiImpl(SpiImpl&& other) noexcept;
    SpiImpl& operator=(SpiImpl&& other) noexcept;

    ~SpiImpl() override;

    // ISpiインターフェースの実装
    void WriteBytes(const uint8_t* data, size_t len) override;
    void ReadBytes(uint8_t* buffer, size_t len) override;
    void Transfer(const uint8_t* tx_data, uint8_t* rx_buffer, size_t len) override;

private:
    int fd_;
};

}  // namespace hal

#endif  // CYCOM_HAL_IMPL_SPI_IMPL_H_
