#include "hal/spi_controller.h"

#include <stdexcept>
#include <unistd.h>       // ::write, ::close
#include <fcntl.h>        // ::open, O_RDWR
#include <sys/ioctl.h>    // ::ioctl
#include <linux/spi/spidev.h>  // SPI_IOC_*

namespace hal {

SPI::SPI(const char* dev, const uint32_t &speed_hz, const uint8_t &mode, const uint8_t &bits)
    : fd_(-1)
{
    fd_ = ::open(dev, O_RDWR);
    if (fd_ < 0) throw std::runtime_error("open spidev failed");

    if (::ioctl(fd_, SPI_IOC_WR_MODE, &mode) < 0)
        throw std::runtime_error("SPI set mode failed");
    if (::ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0)
        throw std::runtime_error("SPI set bits failed");
    if (::ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) < 0)
        throw std::runtime_error("SPI set speed failed");
}

SPI::SPI(SPI&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

SPI& SPI::operator=(SPI&& other) noexcept {
    if (this != &other) {
        if (fd_ >= 0) ::close(fd_);
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

void SPI::WriteBytes(const uint8_t* data, const size_t &len) {
    ssize_t w = ::write(fd_, data, len);
    if (w != static_cast<ssize_t>(len))
        throw std::runtime_error("SPI write failed");
}

SPI::~SPI() {
    if (fd_ >= 0) ::close(fd_);
}

}  // namespace hal