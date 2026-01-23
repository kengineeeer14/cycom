#include "hal/impl/spi_impl.h"

namespace hal {

// モック実装（テスト環境用）
SpiImpl::SpiImpl(const char* dev, uint32_t speed_hz, uint8_t mode, uint8_t bits)
    : fd_(1) {  // ダミーfd
}

SpiImpl::SpiImpl(SpiImpl&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

SpiImpl& SpiImpl::operator=(SpiImpl&& other) noexcept {
    if (this != &other) {
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

void SpiImpl::WriteBytes(const uint8_t* data, size_t len) {
    // モック: 何もしない
}

void SpiImpl::ReadBytes(uint8_t* buffer, size_t len) {
    // モック: バッファを0で埋める
    for (size_t i = 0; i < len; ++i) {
        buffer[i] = 0;
    }
}

void SpiImpl::Transfer(const uint8_t* tx_data, uint8_t* rx_buffer, size_t len) {
    // モック: 受信バッファを0で埋める
    for (size_t i = 0; i < len; ++i) {
        rx_buffer[i] = 0;
    }
}

SpiImpl::~SpiImpl() {
    // モック: 何もしない
}

}  // namespace hal
