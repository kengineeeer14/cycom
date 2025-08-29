#ifndef I2C_CONTROLLER_H_
#define I2C_CONTROLLER_H_

#include <cstdint>
#include <cstddef>
#include <stdexcept>

// I2C 通信を行うためのヘルパークラス。
//   - /dev/i2c-* デバイスを open して使用する。
//   - 16bit レジスタアドレス付きの read/write を提供。
//   - 8bit 読み書き用のショートカットも用意。
class I2C {
public:
    // I2C デバイスを開き、互換のため I2C_SLAVE を設定(実際は I2C_RDWR を使用)。
    I2C(const char* dev, uint8_t addr);

    // 16bit レジスタアドレスに任意長データを書き込む。
    void writeReg16(uint16_t reg, const uint8_t* data, size_t len);

    // 16bit レジスタアドレスを先に書いてから、Repeated Start で len バイト読む。
    void readReg16(uint16_t reg, uint8_t* out, size_t len);

    // 1バイト読み出しのショートカット。
    uint8_t read8(uint16_t reg);

    // 1バイト書き込みのショートカット。
    void write8(uint16_t reg, uint8_t v);

    ~I2C();

private:
    int fd_;
    uint8_t addr_;
};

#endif  // I2C_CONTROLLER_H_