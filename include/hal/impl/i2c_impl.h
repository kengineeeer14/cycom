#ifndef CYCOM_HAL_IMPL_I2C_IMPL_H_
#define CYCOM_HAL_IMPL_I2C_IMPL_H_

#include <cstdint>
#include <cstddef>
#include "hal/interface/i_i2c.h"

namespace hal {

/**
 * @brief I2C通信の実装クラス（Linux I2Cデバイス使用）
 * 
 * II2cインターフェースを実装し、Raspberry PiのI2Cバスを制御します。
 * /dev/i2c-*デバイスファイルを使用した実ハードウェア実装です。
 */
class I2cImpl : public II2c {
public:
    /**
     * @brief I2Cデバイスを初期化する
     * 
     * @param dev I2Cデバイスファイルのパス（例: "/dev/i2c-1"）
     * @param addr 接続先スレーブデバイスの7ビットアドレス
     */
    I2cImpl(const char* dev, uint8_t addr);

    ~I2cImpl() override;

    // II2cインターフェースの実装
    bool WriteByte(uint8_t addr, uint8_t reg, uint8_t value) override;
    bool ReadByte(uint8_t addr, uint8_t reg, uint8_t& value) override;
    bool ReadBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t len) override;
    bool WriteBytes(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) override;
    bool Read16(uint8_t addr, uint16_t reg, uint8_t* buffer, size_t len) override;
    bool Write16(uint8_t addr, uint16_t reg, const uint8_t* data, size_t len) override;

private:
    int fd_;
    uint8_t addr_;
};

}  // namespace hal

#endif  // CYCOM_HAL_IMPL_I2C_IMPL_H_
