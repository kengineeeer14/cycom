#ifndef HAL_SPI_CONFIG_H_
#define HAL_SPI_CONFIG_H_

#include <cstdint>
#include <cstddef>

namespace hal {

class SPI {
public:
    // spidev デバイスを開き、SPI モード/ビット幅/クロックを設定。
    explicit SPI(const char* dev,
                 uint32_t speed_hz = 40000000,
                 uint8_t mode = 0,
                 uint8_t bits = 8);

    // コピー禁止、ムーブのみ許可
    SPI(const SPI&) = delete;
    SPI& operator=(const SPI&) = delete;
    SPI(SPI&& other) noexcept;
    SPI& operator=(SPI&& other) noexcept;

    // 指定バッファを一括送信。送信バイト数が一致しない場合は例外。
    void writeBytes(const uint8_t* data, size_t len);

    // デストラクタでクローズ
    ~SPI();

private:
    int fd_;
};

}  // namespace hal

#endif  // HAL_SPI_CONFIG_H_