#include "hal/impl/spi_impl.h"

#ifdef USE_HARDWARE
#include <stdexcept>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>
#endif

namespace hal {

#ifdef USE_HARDWARE

SpiImpl::SpiImpl(const char* dev, uint32_t speed_hz, uint8_t mode, uint8_t bits)
    : fd_(-1) {
    fd_ = ::open(dev, O_RDWR);
    if (fd_ < 0) throw std::runtime_error("open spidev failed");

    if (::ioctl(fd_, SPI_IOC_WR_MODE, &mode) < 0)
        throw std::runtime_error("SPI set mode failed");
    if (::ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0)
        throw std::runtime_error("SPI set bits failed");
    if (::ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) < 0)
        throw std::runtime_error("SPI set speed failed");
}

SpiImpl::SpiImpl(SpiImpl&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

SpiImpl& SpiImpl::operator=(SpiImpl&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) ::close(fd_);
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

void SpiImpl::WriteBytes(const uint8_t* data, size_t len) {
    ssize_t w = ::write(fd_, data, len);
    if (w != static_cast<ssize_t>(len))
        throw std::runtime_error("SPI write failed");
}

void SpiImpl::ReadBytes(uint8_t* buffer, size_t len) {
    ssize_t r = ::read(fd_, buffer, len);
    if (r != static_cast<ssize_t>(len))
        throw std::runtime_error("SPI read failed");
}

void SpiImpl::Transfer(const uint8_t* tx_data, uint8_t* rx_buffer, size_t len) {
    struct spi_ioc_transfer tr = {};
    tr.tx_buf = reinterpret_cast<__u64>(tx_data);
    tr.rx_buf = reinterpret_cast<__u64>(rx_buffer);
    tr.len = len;
    
    if (::ioctl(fd_, SPI_IOC_MESSAGE(1), &tr) < 0)
        throw std::runtime_error("SPI transfer failed");
}

SpiImpl::~SpiImpl() {
    if (fd_ >= 0) ::close(fd_);
}

#else

// モック実装（開発環境用）
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

#endif

}  // namespace hal
