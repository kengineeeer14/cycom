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
        const std::string font_path = "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf";
        if (access(font_path.c_str(), F_OK) != 0) {
            GTEST_SKIP() << "Font file not found: " << font_path;
        }
    }

    void TearDown() override {}

    driver::MockDisplay mock_display;
    TextRenderer text_renderer{mock_display, "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf"};
};

// Blend565のユニットテスト
TEST_F(TextRendererTest, Blend565_FullyOpaque) {
    // アルファ = 255（完全不透明）の場合、前景色がそのまま返る
    const uint16_t bg{0xF800};  // 赤（RGB565）
    const uint16_t fg{0x07E0};  // 緑（RGB565）
    const uint16_t result{text_renderer.Blend565(bg, fg, 255)};
    EXPECT_EQ(result, fg);
}

}  // namespace ui