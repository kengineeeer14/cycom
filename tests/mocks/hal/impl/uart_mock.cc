#include "hal/impl/uart_impl.h"

namespace hal {

// モック実装（テスト環境用）
UartImpl::UartImpl(const std::string& port, unsigned int baudrate) : fd_(1) {}

int UartImpl::GetFileDescriptor() {
    return fd_;
}

ssize_t UartImpl::Read(uint8_t* buffer, size_t len) {
    // モック: 何も読まない
    return 0;
}

ssize_t UartImpl::Write(const uint8_t* data, size_t len) {
    // モック: 常に成功
    return static_cast<ssize_t>(len);
}

bool UartImpl::IsOpen() {
    return fd_ > 0;
}

UartImpl::~UartImpl() {}

}  // namespace hal
