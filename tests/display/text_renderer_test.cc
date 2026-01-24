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

    const std::string font_path{"/workspace/config/fonts/DejaVuSans.ttf"};
    driver::MockDisplay mock_display;
    TextRenderer text_renderer{mock_display, font_path};
};

// Blend565のユニットテスト
// 要件：アルファ値に基づいて、背景色と前景色を正しく合成できること
TEST_F(TextRendererTest, Blend565_FullyOpaque) {
    // アルファ = 255（完全不透明）の場合、前景色がそのまま返る
    const uint16_t background{0xF800};  // 赤（RGB565）
    const uint16_t foreground{0x07E0};  // 緑（RGB565）
    const uint16_t result{text_renderer.Blend565(background, foreground, 255)};
    EXPECT_EQ(result, foreground);
}

TEST_F(TextRendererTest, Blend565_FullyTransparent) {
    // アルファ = 0（完全透明）の場合、背景色がそのまま返る
    const uint16_t background{0xF800};  // 赤（RGB565）
    const uint16_t foreground{0x07E0};  // 緑（RGB565）
    const uint16_t result{text_renderer.Blend565(background, foreground, 0)};
    EXPECT_EQ(result, background);
}

TEST_F(TextRendererTest, Blend565_HalfTransparent) {
    // アルファ = 128（半透明）の場合、背景色と前景色が50:50で合成される
    const uint16_t background{0x0000};  // 黒（RGB565）
    const uint16_t foreground{0xFFFF};  // 白（RGB565）
    const uint16_t result{text_renderer.Blend565(background, foreground, 128)};

    // 各色成分が約半分になることを確認
    const int result_red{text_renderer.ExtractColorComponent(result, 11, 0x1F)};
    const int result_green{text_renderer.ExtractColorComponent(result, 5, 0x3F)};
    const int result_blue{text_renderer.ExtractColorComponent(result, 0, 0x1F)};

    // 白(0xFFFF)の50%で合成
    EXPECT_EQ(result_red, ((0xFFFF >> 11) & 0x1F) / 2);   // R成分の半分
    EXPECT_EQ(result_green, ((0xFFFF >> 5) & 0x3F) / 2);  // G成分の半分
    EXPECT_EQ(result_blue, ((0xFFFF >> 0) & 0x1F) / 2);   // B成分の半分
}

TEST_F(TextRendererTest, Blend565_SameColor) {
    // 背景色と前景色が同じ場合、アルファ値に関わらず同じ色が返る
    const uint16_t color{0x07E0};  // 緑（RGB565）
    const uint16_t result_opaque{text_renderer.Blend565(color, color, 255)};
    const uint16_t result_transparent{text_renderer.Blend565(color, color, 0)};
    const uint16_t result_half{text_renderer.Blend565(color, color, 128)};

    EXPECT_EQ(result_opaque, color);
    EXPECT_EQ(result_transparent, color);
    EXPECT_EQ(result_half, color);
}

TEST_F(TextRendererTest, Blend565_MaxColorComponents) {
    // RGB565の各成分が最大値の場合
    const uint16_t white{0xFFFF};  // R=31, G=63, B=31
    const uint16_t black{0x0000};  // R=0, G=0, B=0

    // alpha=64（約25%）でブレンド
    const uint16_t result{text_renderer.Blend565(black, white, 64)};

    const int result_red{text_renderer.ExtractColorComponent(result, 11, 0x1F)};
    const int result_green{text_renderer.ExtractColorComponent(result, 5, 0x3F)};
    const int result_blue{text_renderer.ExtractColorComponent(result, 0, 0x1F)};

    // 白(0xFFFF)の25%で合成
    EXPECT_EQ(result_red, ((0xFFFF >> 11) & 0x1F) / 4);   // R成分の25%
    EXPECT_EQ(result_green, ((0xFFFF >> 5) & 0x3F) / 4);  // G成分の25%
    EXPECT_EQ(result_blue, ((0xFFFF >> 0) & 0x1F) / 4);   // B成分の25%
}

TEST_F(TextRendererTest, Blend565_PrimaryColors) {
    // 基本色（赤・緑・青）のブレンドテスト
    const uint16_t red{0xF800};    // R=31, G=0, B=0
    const uint16_t green{0x07E0};  // R=0, G=63, B=0
    const uint16_t blue{0x001F};   // R=0, G=0, B=31

    // 赤と緑を50:50でブレンド → 黄色系
    const uint16_t red_green{text_renderer.Blend565(red, green, 128)};
    const int rg_red{text_renderer.ExtractColorComponent(red_green, 11, 0x1F)};
    const int rg_green{text_renderer.ExtractColorComponent(red_green, 5, 0x3F)};
    const int rg_blue{text_renderer.ExtractColorComponent(red_green, 0, 0x1F)};

    EXPECT_EQ(rg_red, ((0xF800 >> 11) & 0x1F) / 2);   // 赤成分の半分
    EXPECT_EQ(rg_green, ((0x07E0 >> 5) & 0x3F) / 2);  // 緑成分の半分
    EXPECT_EQ(rg_blue, 0);                            // 青成分はゼロ

    // 緑と青を50:50でブレンド → シアン系
    const uint16_t green_blue{text_renderer.Blend565(green, blue, 128)};
    const int gb_red{text_renderer.ExtractColorComponent(green_blue, 11, 0x1F)};
    const int gb_green{text_renderer.ExtractColorComponent(green_blue, 5, 0x3F)};
    const int gb_blue{text_renderer.ExtractColorComponent(green_blue, 0, 0x1F)};

    EXPECT_EQ(gb_red, 0);                             // 赤成分はゼロ
    EXPECT_EQ(gb_green, ((0x07E0 >> 5) & 0x3F) / 2);  // 緑成分の半分
    EXPECT_EQ(gb_blue, ((0x001F >> 0) & 0x1F) / 2);   // 青成分の半分

    // 青と赤を75:25でブレンド → 紫系
    const uint16_t blue_red{text_renderer.Blend565(blue, red, 64)};
    const int br_red{text_renderer.ExtractColorComponent(blue_red, 11, 0x1F)};
    const int br_green{text_renderer.ExtractColorComponent(blue_red, 5, 0x3F)};
    const int br_blue{text_renderer.ExtractColorComponent(blue_red, 0, 0x1F)};

    EXPECT_EQ(br_red, ((0xF800 >> 11) & 0x1F) / 4);      // 赤成分の25%
    EXPECT_EQ(br_green, 0);                              // 緑成分はゼロ
    EXPECT_EQ(br_blue, ((0x001F >> 0) & 0x1F) * 3 / 4);  // 青成分の75%
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