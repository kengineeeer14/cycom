#ifndef CYCOM_TESTS_MOCKS_DRIVER_MOCK_DISPLAY_H_
#define CYCOM_TESTS_MOCKS_DRIVER_MOCK_DISPLAY_H_

#include "driver/interface/i_display.h"

namespace driver {

/**
 * @brief IDisplayのモック実装
 *
 * テスト用のシンプルなディスプレイモック。
 * 実際の描画は行わず、メソッド呼び出しを記録する。
 */
class MockDisplay : public IDisplay {
  public:
    MockDisplay() = default;
    ~MockDisplay() override = default;

    void Clear(uint16_t rgb565 = 0xFFFF) override {
        // 何もしない
    }

    void DrawRGB565Line(int x, int y, const uint16_t* rgb565, int len) override {
        // 何もしない
    }

    bool DrawBackgroundImage(const std::string& path) override {
        return true;  // 常に成功とする
    }

    int GetWidth() const override {
        return 320;  // 固定値
    }

    int GetHeight() const override {
        return 480;  // 固定値
    }
};

}  // namespace driver

#endif  // CYCOM_TESTS_MOCKS_DRIVER_MOCK_DISPLAY_H_
