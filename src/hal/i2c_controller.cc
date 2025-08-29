#include "hal/i2c_controller.h"

#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <vector>
#include <cstring>
#include <stdexcept>

namespace hal{

I2C::I2C(const char* dev, const uint8_t &addr) : fd_(-1), addr_(addr) {
    fd_ = ::open(dev, O_RDWR);
    if (fd_ < 0) throw std::runtime_error("open i2c failed");
    // I2C_RDWR で毎回 addr を指定するので必須ではないが、互換のため設定
    if (ioctl(fd_, I2C_SLAVE, addr_) < 0) throw std::runtime_error("I2C_SLAVE failed");
}

void I2C::WriteReg16(const uint16_t &reg, const uint8_t* data, const size_t &len) {
    // レジスタアドレス（上位・下位バイト）と任意のデータ列を連結し、送信用のバッファを作成
    std::vector<uint8_t> buf(2 + len);
    buf[0] = static_cast<uint8_t>(reg >> 8);
    buf[1] = static_cast<uint8_t>(reg & 0xFF);
    std::memcpy(buf.data() + 2, data, len);

    // I2C通信で送るメッセージの構造体を準備
    i2c_msg msg{};
    msg.addr  = addr_;
    msg.flags = 0;
    msg.len   = static_cast<__u16>(buf.size());
    msg.buf   = buf.data();

    // I²C ドライバに対して、指定した1本のメッセージを送信するためのパラメータを準備
    i2c_rdwr_ioctl_data ioctl_data{};
    ioctl_data.msgs  = &msg;
    ioctl_data.nmsgs = 1;
    
    // カーネルに I²C 転送を依頼し、失敗したら例外処理
    if (ioctl(fd_, I2C_RDWR, &ioctl_data) < 0) throw std::runtime_error("I2C_RDWR write failed");
}

void I2C::ReadReg16(const uint16_t &reg, uint8_t* out, const size_t &len) {
    uint8_t addrbuf[2] = {
        static_cast<uint8_t>(reg >> 8),
        static_cast<uint8_t>(reg & 0xFF)
    };

    i2c_msg msgs[2]{};
    // I2C スレーブデバイスに、後続の読み出し操作において参照すべきレジスタアドレスを送信し、読み出し対象を指定
    msgs[0].addr  = addr_;
    msgs[0].flags = 0;
    msgs[0].len   = 2;
    msgs[0].buf   = addrbuf;
    // スレーブデバイスから len バイトのデータを読み出し、指定されたバッファ out に格納
    msgs[1].addr  = addr_;
    msgs[1].flags = I2C_M_RD;
    msgs[1].len   = static_cast<__u16>(len);
    msgs[1].buf   = out;

    i2c_rdwr_ioctl_data ioctl_data{};
    ioctl_data.msgs  = msgs;
    ioctl_data.nmsgs = 2;

    // I2C メッセージ（レジスタアドレス書き込みとデータ読み出し）をまとめてカーネルに転送依頼し、失敗した場合は例外処理
    if (ioctl(fd_, I2C_RDWR, &ioctl_data) < 0) throw std::runtime_error("I2C_RDWR read failed");
}

uint8_t I2C::Read8(const uint16_t &reg) {
    uint8_t v{};
    ReadReg16(reg, &v, 1);
    return v;
}

void I2C::Write8(const uint16_t &reg, const uint8_t &v) {
    WriteReg16(reg, &v, 1);
}

I2C::~I2C() {
    if (fd_ >= 0) ::close(fd_);
}

} // namespace hal