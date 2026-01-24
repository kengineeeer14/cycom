#include "display/text_renderer.h"

#include "mocks/driver/mock_display.h"

#include <gtest/gtest.h>
#include <unistd.h>

namespace ui {

// TextRendererのためのテストフィクスチャ
class TextRendererTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // フォントファイルがない場合はスキップ
        if (access(font_path.c_str(), F_OK) != 0) {
            GTEST_SKIP() << "Font file not found: " << font_path;
        }
    }

    void TearDown() override {}

    const std::string font_path{"/workspace/config/fonts/DejaVuSans.ttf"};
    driver::MockDisplay mock_display;
    TextRenderer text_renderer{mock_display, font_path};
};

// Blend565のユニットテスト
TEST_F(TextRendererTest, Blend565_FullyOpaque) {
    // アルファ = 255（完全不透明）の場合、前景色がそのまま返る
    const uint16_t bg{0xF800};  // 赤（RGB565）
    const uint16_t fg{0x07E0};  // 緑（RGB565）
    const uint16_t result{text_renderer.Blend565(bg, fg, 255)};
    EXPECT_EQ(result, fg);
}

// ExtractColorComponentのユニットテスト
// 要件：RGB565形式の色から，指定された色成分（赤・緑・青）を抽出できること
TEST_F(TextRendererTest, ExtractColorComponentTest) {
    const uint16_t color{0xABCD};  // RGB565

    // [R R R R R][G G G G G G][B B B B B]
    // ビット15-11  ビット10-5   ビット4-0

    // 赤成分: 0x15 (0b10101)を抽出
    // 11ビットずらして，0x1F (0b11111)でマスク
    const int r{text_renderer.ExtractColorComponent(color, 11, 0x1F)};
    // 緑成分: 0x1E (0b011110)
    // 5ビットずらして，0x3F (0b111111)でマスク
    const int g{text_renderer.ExtractColorComponent(color, 5, 0x3F)};
    // 青成分: 0x0D (0b01101)
    // 0ビットずらして，0x1F (0b11111)でマスク
    const int b{text_renderer.ExtractColorComponent(color, 0, 0x1F)};
    EXPECT_EQ(r, 0b10101);   // 赤成分
    EXPECT_EQ(g, 0b011110);  // 緑成分
    EXPECT_EQ(b, 0b01101);   // 青成分
}

}  // namespace ui