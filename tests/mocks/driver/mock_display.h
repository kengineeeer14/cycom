#ifndef CYCOM_TESTS_MOCKS_DRIVER_MOCK_DISPLAY_H_
#define CYCOM_TESTS_MOCKS_DRIVER_MOCK_DISPLAY_H_

#include "driver/interface/i_display.h"

#include <gmock/gmock.h>

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

    MOCK_METHOD(void, Clear, (uint16_t rgb565), (override));
    MOCK_METHOD(void, DrawRGB565Line, (int x, int y, const uint16_t *rgb565, int len), (override));
    MOCK_METHOD(bool, DrawBackgroundImage, (const std::string &path), (override));
    MOCK_METHOD(int, GetWidth, (), (const, override));
    MOCK_METHOD(int, GetHeight, (), (const, override));
};

}  // namespace driver

#endif  // CYCOM_TESTS_MOCKS_DRIVER_MOCK_DISPLAY_H_
