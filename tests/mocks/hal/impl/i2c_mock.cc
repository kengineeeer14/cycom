#include "hal/impl/i2c_impl.h"

namespace hal {

// モック実装（テスト環境用）
I2cImpl::I2cImpl(const char* dev, uint8_t addr) : fd_(1), addr_(addr) {}

bool I2cImpl::WriteByte(uint8_t addr, uint8_t reg, uint8_t value) {
    return true;
}

bool I2cImpl::ReadByte(uint8_t addr, uint8_t reg, uint8_t& value) {
    value = 0;
    return true;
}

bool I2cImpl::ReadBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t len) {
    for (size_t i = 0; i < len; ++i)
        buffer[i] = 0;
    return true;
}

bool I2cImpl::WriteBytes(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
    return true;
}

bool I2cImpl::Read16(uint8_t addr, uint16_t reg, uint8_t* buffer, size_t len) {
    for (size_t i = 0; i < len; ++i)
        buffer[i] = 0;
    return true;
}

bool I2cImpl::Write16(uint8_t addr, uint16_t reg, const uint8_t* data, size_t len) {
    return true;
}

I2cImpl::~I2cImpl() {}

}  // namespace hal
