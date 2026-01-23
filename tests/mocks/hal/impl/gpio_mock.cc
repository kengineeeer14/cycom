#include "hal/impl/gpio_impl.h"

namespace hal {

// モック実装（テスト環境用）
gpiod_chip* GpioImpl::OpenChipFlexible(const std::string& chip) {
    return reinterpret_cast<gpiod_chip*>(1);  // ダミーポインタ
}

GpioImpl::GpioImpl(const std::string& chip, unsigned int offset, bool output, int initial)
    : is_output_(output) {
    chip_ = OpenChipFlexible(chip);
    line_ = reinterpret_cast<gpiod_line*>(1);  // ダミーポインタ
}

void GpioImpl::Set(int value) {
    // モック: 何もしない
}

int GpioImpl::Get() {
    // モック: 常に0を返す
    return 0;
}

void GpioImpl::RequestRisingEdge() {
    // モック: 何もしない
}

void GpioImpl::RequestFallingEdge() {
    // モック: 何もしない
}

bool GpioImpl::WaitForEvent(int timeout_sec) {
    // モック: 常にfalseを返す（タイムアウト）
    return false;
}

void GpioImpl::ReadEvent(gpiod_line_event& ev) {
    // モック: ダミーイベント
    ev.event_type = 0;
    ev.ts = {0, 0};
}

GpioImpl::~GpioImpl() {
    // モック: 何もしない
}

}  // namespace hal
