#include "hal/impl/i2c_impl.h"

#include <cstring>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

namespace hal {

I2cImpl::I2cImpl(const char* dev, uint8_t addr) : fd_(-1), addr_(addr) {
    fd_ = ::open(dev, O_RDWR);
    if (fd_ < 0)
        throw std::runtime_error("open i2c failed");
    if (ioctl(fd_, I2C_SLAVE, addr_) < 0) {
        ::close(fd_);
        throw std::runtime_error("I2C_SLAVE failed");
    }
}

bool I2cImpl::WriteByte(uint8_t addr, uint8_t reg, uint8_t value) {
    return WriteBytes(addr, reg, &value, 1);
}

bool I2cImpl::ReadByte(uint8_t addr, uint8_t reg, uint8_t& value) {
    return ReadBytes(addr, reg, &value, 1);
}

bool I2cImpl::ReadBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t len) {
    i2c_msg msgs[2]{};
    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = 1;
    msgs[0].buf = &reg;

    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = static_cast<__u16>(len);
    msgs[1].buf = buffer;

    i2c_rdwr_ioctl_data ioctl_data{};
    ioctl_data.msgs = msgs;
    ioctl_data.nmsgs = 2;

    return ioctl(fd_, I2C_RDWR, &ioctl_data) >= 0;
}

bool I2cImpl::WriteBytes(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) {
    std::vector<uint8_t> buf(1 + len);
    buf[0] = reg;
    std::memcpy(buf.data() + 1, data, len);

    i2c_msg msg{};
    msg.addr = addr;
    msg.flags = 0;
    msg.len = static_cast<__u16>(buf.size());
    msg.buf = buf.data();

    i2c_rdwr_ioctl_data ioctl_data{};
    ioctl_data.msgs = &msg;
    ioctl_data.nmsgs = 1;

    return ioctl(fd_, I2C_RDWR, &ioctl_data) >= 0;
}

bool I2cImpl::Read16(uint8_t addr, uint16_t reg, uint8_t* buffer, size_t len) {
    uint8_t addrbuf[2] = {static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF)};

    i2c_msg msgs[2]{};
    msgs[0].addr = addr;
    msgs[0].flags = 0;
    msgs[0].len = 2;
    msgs[0].buf = addrbuf;

    msgs[1].addr = addr;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len = static_cast<__u16>(len);
    msgs[1].buf = buffer;

    i2c_rdwr_ioctl_data ioctl_data{};
    ioctl_data.msgs = msgs;
    ioctl_data.nmsgs = 2;

    return ioctl(fd_, I2C_RDWR, &ioctl_data) >= 0;
}

bool I2cImpl::Write16(uint8_t addr, uint16_t reg, const uint8_t* data, size_t len) {
    std::vector<uint8_t> buf(2 + len);
    buf[0] = static_cast<uint8_t>(reg >> 8);
    buf[1] = static_cast<uint8_t>(reg & 0xFF);
    std::memcpy(buf.data() + 2, data, len);

    i2c_msg msg{};
    msg.addr = addr;
    msg.flags = 0;
    msg.len = static_cast<__u16>(buf.size());
    msg.buf = buf.data();

    i2c_rdwr_ioctl_data ioctl_data{};
    ioctl_data.msgs = &msg;
    ioctl_data.nmsgs = 1;

    return ioctl(fd_, I2C_RDWR, &ioctl_data) >= 0;
}

I2cImpl::~I2cImpl() {
    if (fd_ >= 0)
        ::close(fd_);
}

}  // namespace hal
