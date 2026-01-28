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

// =============================================================
// Blend565のユニットテスト
// -------------------------------------------------------------
// 要件：アルファ値に基づいて、背景色と前景色を正しく合成できること
// =============================================================
TEST_F(TextRendererTest, Blend565_FullyOpaque) {
    // アルファ = 255（完全不透明）の場合、前景色がそのまま返る
    const uint16_t background{0xF800};  // 赤（RGB565）
    const uint16_t foreground{0x07E0};  // 緑（RGB565）
    const uint16_t result{text_renderer.Blend565(background, foreground, TextRenderer::kAlphaMax)};
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
    const uint16_t result{text_renderer.Blend565(background, foreground, TextRenderer::kAlphaMax / 2)};

    // 各色成分が約半分になることを確認
    const int result_red{text_renderer.ExtractColorComponent(result, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    const int result_green{text_renderer.ExtractColorComponent(result, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    const int result_blue{text_renderer.ExtractColorComponent(result, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};

    // 白(0xFFFF)の50%で合成
    EXPECT_EQ(result_red, ((0xFFFF >> TextRenderer::kRedShift) & TextRenderer::kRedMask) / 2);        // R成分の半分
    EXPECT_EQ(result_green, ((0xFFFF >> TextRenderer::kGreenShift) & TextRenderer::kGreenMask) / 2);  // G成分の半分
    EXPECT_EQ(result_blue, ((0xFFFF >> TextRenderer::kBlueShift) & TextRenderer::kBlueMask) / 2);     // B成分の半分
}

TEST_F(TextRendererTest, Blend565_MaxColorComponents) {
    // RGB565の各成分が最大値の場合
    const uint16_t white{0xFFFF};  // R=31, G=63, B=31
    const uint16_t black{0x0000};  // R=0, G=0, B=0

    // alpha=64（約25%）でブレンド
    const uint16_t result{text_renderer.Blend565(black, white, TextRenderer::kAlphaMax / 4)};

    const int result_red{text_renderer.ExtractColorComponent(result, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    const int result_green{text_renderer.ExtractColorComponent(result, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    const int result_blue{text_renderer.ExtractColorComponent(result, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};

    // 白(0xFFFF)の25%で合成
    EXPECT_EQ(result_red, ((0xFFFF >> TextRenderer::kRedShift) & TextRenderer::kRedMask) / 4);        // R成分の25%
    EXPECT_EQ(result_green, ((0xFFFF >> TextRenderer::kGreenShift) & TextRenderer::kGreenMask) / 4);  // G成分の25%
    EXPECT_EQ(result_blue, ((0xFFFF >> TextRenderer::kBlueShift) & TextRenderer::kBlueMask) / 4);     // B成分の25%
}

TEST_F(TextRendererTest, Blend565_PrimaryColors) {
    // 基本色（赤・緑・青）のブレンドテスト
    const uint16_t red{0xF800};    // R=31, G=0, B=0
    const uint16_t green{0x07E0};  // R=0, G=63, B=0
    const uint16_t blue{0x001F};   // R=0, G=0, B=31

    // 赤と緑を50:50でブレンド → 黄色系
    const uint16_t red_green{text_renderer.Blend565(red, green, TextRenderer::kAlphaMax / 2)};
    const int rg_red{text_renderer.ExtractColorComponent(red_green, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    const int rg_green{text_renderer.ExtractColorComponent(red_green, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    const int rg_blue{text_renderer.ExtractColorComponent(red_green, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};

    EXPECT_EQ(rg_red, ((0xF800 >> TextRenderer::kRedShift) & TextRenderer::kRedMask) / 2);        // 赤成分の半分
    EXPECT_EQ(rg_green, ((0x07E0 >> TextRenderer::kGreenShift) & TextRenderer::kGreenMask) / 2);  // 緑成分の半分
    EXPECT_EQ(rg_blue, 0);                                                                        // 青成分はゼロ

    // 緑と青を50:50でブレンド → シアン系
    const uint16_t green_blue{text_renderer.Blend565(green, blue, TextRenderer::kAlphaMax / 2)};
    const int gb_red{text_renderer.ExtractColorComponent(green_blue, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    const int gb_green{text_renderer.ExtractColorComponent(green_blue, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    const int gb_blue{text_renderer.ExtractColorComponent(green_blue, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};

    EXPECT_EQ(gb_red, 0);                                                                         // 赤成分はゼロ
    EXPECT_EQ(gb_green, ((0x07E0 >> TextRenderer::kGreenShift) & TextRenderer::kGreenMask) / 2);  // 緑成分の半分
    EXPECT_EQ(gb_blue, ((0x001F >> TextRenderer::kBlueShift) & TextRenderer::kBlueMask) / 2);     // 青成分の半分

    // 青と赤を75:25でブレンド → 紫系
    const uint16_t blue_red{text_renderer.Blend565(blue, red, TextRenderer::kAlphaMax / 4)};
    const int br_red{text_renderer.ExtractColorComponent(blue_red, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    const int br_green{text_renderer.ExtractColorComponent(blue_red, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    const int br_blue{text_renderer.ExtractColorComponent(blue_red, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};

    EXPECT_EQ(br_red, ((0xF800 >> TextRenderer::kRedShift) & TextRenderer::kRedMask) / 4);         // 赤成分の25%
    EXPECT_EQ(br_green, 0);                                                                        // 緑成分はゼロ
    EXPECT_EQ(br_blue, ((0x001F >> TextRenderer::kBlueShift) & TextRenderer::kBlueMask) * 3 / 4);  // 青成分の75%
}

TEST_F(TextRendererTest, Blend565_SameColor) {
    // 背景色と前景色が同じ場合、アルファ値に関わらず同じ色が返る
    const uint16_t color{0x07E0};  // 緑（RGB565）
    const uint16_t result_opaque{text_renderer.Blend565(color, color, TextRenderer::kAlphaMax)};
    const uint16_t result_transparent{text_renderer.Blend565(color, color, 0)};
    const uint16_t result_half{text_renderer.Blend565(color, color, TextRenderer::kAlphaMax / 2)};

    EXPECT_EQ(result_opaque, color);
    EXPECT_EQ(result_transparent, color);
    EXPECT_EQ(result_half, color);
}

// =============================================================
// ExtractColorComponentのユニットテスト
// -------------------------------------------------------------
// 要件：RGB565形式の色から，指定された色成分（赤・緑・青）を抽出できること
// =============================================================
TEST_F(TextRendererTest, ExtractColorComponentTest) {
    const uint16_t color{0xABCD};  // RGB565

    // [R R R R R][G G G G G G][B B B B B]
    // ビット15-11  ビット10-5   ビット4-0

    // 赤成分: 0x15 (0b10101)を抽出
    // 11ビットずらして，0x1F (0b11111)でマスク
    const int r{text_renderer.ExtractColorComponent(color, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    // 緑成分: 0x1E (0b011110)
    // 5ビットずらして，0x3F (0b111111)でマスク
    const int g{text_renderer.ExtractColorComponent(color, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    // 青成分: 0x0D (0b01101)
    // 0ビットずらして，0x1F (0b11111)でマスク
    const int b{text_renderer.ExtractColorComponent(color, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};
    EXPECT_EQ(r, 0b10101);   // 赤成分
    EXPECT_EQ(g, 0b011110);  // 緑成分
    EXPECT_EQ(b, 0b01101);   // 青成分
}

// =============================================================
// GetCodepointのユニットテスト
// -------------------------------------------------------------
// 要件：
// - UTF-8文字列の現在位置のコードポイントを取得し、インデックスを次の位置に更新すること
// - 不正な文字列が与えられた場合、異常とわかる処置を行うこと
// =============================================================
TEST_F(TextRendererTest, GetCodepoint_AsciiCharacter) {
    // ASCII文字（1バイト）の取得
    const std::string utf8_str{"Hello"};
    size_t index{0};
    uint32_t codepoint{0};

    // 'H' (0x48)
    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x0048);  // Hのコードポイント
    EXPECT_EQ(index, 1);

    // 'e' (0x65)
    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x0065);  // eのコードポイント
    EXPECT_EQ(index, 2);
}

TEST_F(TextRendererTest, GetCodepoint_TwoByteCharacter) {
    // 2バイト文字の取得（ギリシャ文字 α: U+03B1）
    const std::string utf8_str{"α"};  // α (U+03B1)
    size_t index{0};
    uint32_t codepoint{0};

    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x03B1);  // αのコードポイント
    EXPECT_EQ(index, 2);           // 2バイト進む
}

TEST_F(TextRendererTest, GetCodepoint_ThreeByteCharacter) {
    // 3バイト文字の取得（日本語 あ: U+3042）
    const std::string utf8_str{"あ"};  // あ (U+3042)
    size_t index{0};
    uint32_t codepoint{0};

    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x3042);  // あのコードポイント
    EXPECT_EQ(index, 3);           // 3バイト進む
}

TEST_F(TextRendererTest, GetCodepoint_FourByteCharacter) {
    // 4バイト文字の取得（絵文字 🚴: U+1F6B4）
    const std::string utf8_str{"🚴"};  // 🚴 (U+1F6B4)
    size_t index{0};
    uint32_t codepoint{0};

    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x1F6B4);  // 🚴 のコードポイント
    EXPECT_EQ(index, 4);            // 4バイト進む
}

TEST_F(TextRendererTest, GetCodepoint_MixedCharacters) {
    // 混合文字列（ASCII + 3バイト文字）
    const std::string utf8_str{"Aあ"};  // A (1バイト) + あ (3バイト)
    size_t index{0};
    uint32_t codepoint{0};

    // 'A'
    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x0041);  // Aのコードポイント
    EXPECT_EQ(index, 1);           // 1バイト進む

    // 'あ'
    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x3042);  // あのコードポイント
    EXPECT_EQ(index, 4);           // 3バイト進む
}

TEST_F(TextRendererTest, GetCodepoint_EndOfString) {
    // 文字列の終端に達した場合
    const std::string utf8_str{"A"};
    size_t index{1};  // すでに終端
    uint32_t codepoint{0};

    EXPECT_FALSE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
}

TEST_F(TextRendererTest, GetCodepoint_IncompleteTwoByteSequence) {
    // 2バイト文字の途中で終端
    const std::string utf8_str{"\xCE"};  // 2バイト文字の1バイト目のみ
    size_t index{0};
    uint32_t codepoint{0};

    EXPECT_FALSE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(index, 1);  // 1バイト目は読まれている
}

TEST_F(TextRendererTest, GetCodepoint_IncompleteThreeByteSequence) {
    // 3バイト文字の途中で終端（1バイト目のみ）
    const std::string utf8_str{"\xE3"};  // 3バイト文字の1バイト目のみ
    size_t index{0};
    uint32_t codepoint{0};

    // 残り2バイト必要だが、文字列には1バイトしかない
    EXPECT_FALSE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(index, 1);  // 1バイト目は読まれている
}

TEST_F(TextRendererTest, GetCodepoint_IncompleteFourByteSequence) {
    // 4バイト文字の途中で終端（2バイト目まで）
    const std::string utf8_str{"\xF0\x9F"};  // 4バイト文字の2バイト目まで
    size_t index{0};
    uint32_t codepoint{0};

    // 残り3バイト必要だが、文字列には2バイトしかない
    EXPECT_FALSE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(index, 1);  // 1バイト目は読まれている
}

TEST_F(TextRendererTest, GetCodepoint_InvalidSequence) {
    // 不正なUTF-8シーケンス（5バイト文字は存在しない）
    const std::string utf8_str{"\xF8\x80\x80\x80"};  // 無効な先頭バイト
    size_t index{0};
    uint32_t codepoint{0};

    EXPECT_FALSE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, '?');  // 不正な文字は '?' として処理
}

TEST_F(TextRendererTest, GetCodepoint_EmptyString) {
    // 空文字列
    const std::string utf8_str{""};
    size_t index{0};
    uint32_t codepoint{0};

    EXPECT_FALSE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
}

TEST_F(TextRendererTest, GetCodepoint_SequentialCalls) {
    // 複数のコードポイントを順番に取得
    const std::string utf8_str{"こんにちは"};  // 5文字の日本語（各3バイト）
    size_t index{0};
    uint32_t codepoint{0};

    // こ (U+3053)
    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x3053);  // こ のコードポイント
    EXPECT_EQ(index, 3);

    // ん (U+3093)
    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x3093);  // ん のコードポイント
    EXPECT_EQ(index, 6);

    // に (U+306B)
    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x306B);  // に のコードポイント
    EXPECT_EQ(index, 9);

    // ち (U+3061)
    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x3061);  // ち のコードポイント
    EXPECT_EQ(index, 12);

    // は (U+306F)
    EXPECT_TRUE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
    EXPECT_EQ(codepoint, 0x306F);  // は のコードポイント
    EXPECT_EQ(index, 15);

    // 終端
    EXPECT_FALSE(TextRenderer::GetCodepoint(utf8_str, index, codepoint));
}

}  // namespace ui