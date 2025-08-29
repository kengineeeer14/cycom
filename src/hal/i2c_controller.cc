#include "hal/i2c_controller.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <vector>
#include <cstring>

I2C::I2C(const char* dev, uint8_t addr) : fd_(-1), addr_(addr) {
    fd_ = ::open(dev, O_RDWR);
    if (fd_ < 0) throw std::runtime_error("open i2c failed");
    // I2C_RDWR で毎回 addr を指定するので必須ではないが、互換のため設定
    if (ioctl(fd_, I2C_SLAVE, addr_) < 0) throw std::runtime_error("I2C_SLAVE failed");
}

void I2C::writeReg16(uint16_t reg, const uint8_t* data, size_t len) {
    std::vector<uint8_t> buf(2 + len);
    buf[0] = static_cast<uint8_t>(reg >> 8);
    buf[1] = static_cast<uint8_t>(reg & 0xFF);
    std::memcpy(buf.data() + 2, data, len);

    i2c_msg msg{};
    msg.addr  = addr_;
    msg.flags = 0;
    msg.len   = static_cast<__u16>(buf.size());
    msg.buf   = buf.data();

    i2c_rdwr_ioctl_data ioctl_data{};
    ioctl_data.msgs  = &msg;
    ioctl_data.nmsgs = 1;

    if (ioctl(fd_, I2C_RDWR, &ioctl_data) < 0) throw std::runtime_error("I2C_RDWR write failed");
}

void I2C::readReg16(uint16_t reg, uint8_t* out, size_t len) {
    uint8_t addrbuf[2] = {
        static_cast<uint8_t>(reg >> 8),
        static_cast<uint8_t>(reg & 0xFF)
    };

    i2c_msg msgs[2]{};
    // write register address
    msgs[0].addr  = addr_;
    msgs[0].flags = 0;
    msgs[0].len   = 2;
    msgs[0].buf   = addrbuf;
    // read data
    msgs[1].addr  = addr_;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len   = static_cast<__u16>(len);
    msgs[1].buf   = out;

    i2c_rdwr_ioctl_data ioctl_data{};
    ioctl_data.msgs  = msgs;
    ioctl_data.nmsgs = 2;

    if (ioctl(fd_, I2C_RDWR, &ioctl_data) < 0) throw std::runtime_error("I2C_RDWR read failed");
}

uint8_t I2C::read8(uint16_t reg) {
    uint8_t v{};
    readReg16(reg, &v, 1);
    return v;
}

void I2C::write8(uint16_t reg, uint8_t v) {
    writeReg16(reg, &v, 1);
}

I2C::~I2C() {
    if (fd_ >= 0) ::close(fd_);
}