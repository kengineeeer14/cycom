#include "display/text_renderer.h"

#include "mocks/driver/mock_display.h"

#include <gtest/gtest.h>
#include <unistd.h>

namespace ui {

// TextRendererのためのテストフィクスチャ
class TextRendererTest : public ::testing::Test {
  protected:
    void SetUp() override {
        // 各テスト実行前に呼ばれる初期化処理
        if (access(font_path.c_str(), F_OK) != 0) {
            GTEST_SKIP() << "Font file not found: " << font_path;
        }
    }

    void TearDown() override {
        // 各テスト実行後に呼ばれるクリーンアップ処理
    }

    // テストで使用する共通のメンバ変数
    const std::string font_path{"/workspace/config/fonts/DejaVuSans.ttf"};
    driver::MockDisplay mock_display;
    TextRenderer text_renderer{mock_display, font_path};
};

// =============================================================
// MakeKeyのユニットテスト
// -------------------------------------------------------------
// 要件：フォントサイズとコードポイントから一意のキャッシュキーを生成できること
// =============================================================
TEST_F(TextRendererTest, MakeKey_BasicGeneration) {
    // 基本的なキー生成が正しく動作することを確認
    const int size_px{32};
    const uint32_t codepoint{0x3042};  // ひらがな「あ」
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(size_px, codepoint)};

    // キーは size_px を上位ビットに、codepoint を下位21ビットに格納
    // 期待値: (32 << 21) | 0x3042 = 0x0000004000003042
    const TextRenderer::GlyphKey expected{(static_cast<uint64_t>(size_px) << TextRenderer::kCodepointBits) | codepoint};
    EXPECT_EQ(key, expected);
}

TEST_F(TextRendererTest, MakeKey_DifferentSizes) {
    // 同じコードポイントでも異なるサイズで異なるキーが生成されることを確認
    const uint32_t codepoint{0x0041};  // 'A'
    const TextRenderer::GlyphKey key_16{TextRenderer::MakeKey(16, codepoint)};
    const TextRenderer::GlyphKey key_32{TextRenderer::MakeKey(32, codepoint)};
    const TextRenderer::GlyphKey key_64{TextRenderer::MakeKey(64, codepoint)};

    // すべてのキーが異なることを確認
    EXPECT_NE(key_16, key_32);
    EXPECT_NE(key_32, key_64);
    EXPECT_NE(key_16, key_64);
}

TEST_F(TextRendererTest, MakeKey_DifferentCodepoints) {
    // 同じサイズでも異なるコードポイントで異なるキーが生成されることを確認
    const int size_px{32};
    const TextRenderer::GlyphKey key_a{TextRenderer::MakeKey(size_px, 0x0041)};   // 'A'
    const TextRenderer::GlyphKey key_b{TextRenderer::MakeKey(size_px, 0x0042)};   // 'B'
    const TextRenderer::GlyphKey key_ja{TextRenderer::MakeKey(size_px, 0x3042)};  // 'あ'

    // すべてのキーが異なることを確認
    EXPECT_NE(key_a, key_b);
    EXPECT_NE(key_b, key_ja);
    EXPECT_NE(key_a, key_ja);
}

TEST_F(TextRendererTest, MakeKey_MaxCodepoint) {
    // Unicodeの最大コードポイント（U+10FFFF）でキーを生成
    const int size_px{32};
    const uint32_t max_codepoint{0x10FFFF};
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(size_px, max_codepoint)};

    // コードポイント部分（下位21ビット）が正しくマスクされていることを確認
    const uint32_t extracted_codepoint{static_cast<uint32_t>(key & TextRenderer::kCodepointMask)};
    EXPECT_EQ(extracted_codepoint, max_codepoint);

    // サイズ部分（上位ビット）が正しく格納されていることを確認
    const int extracted_size{static_cast<int>(key >> TextRenderer::kCodepointBits)};
    EXPECT_EQ(extracted_size, size_px);
}

TEST_F(TextRendererTest, MakeKey_CodepointMasking) {
    // コードポイントが21ビットを超える場合、マスクが適用されることを確認
    const int size_px{32};
    const uint32_t invalid_codepoint{0xFFFFFFFF};  // 全ビット1
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(size_px, invalid_codepoint)};

    // 下位21ビットのみが保持される
    const uint32_t extracted_codepoint{static_cast<uint32_t>(key & TextRenderer::kCodepointMask)};
    EXPECT_EQ(extracted_codepoint, TextRenderer::kCodepointMask);  // 0x1FFFFF
}

TEST_F(TextRendererTest, MakeKey_LargeSize) {
    // 大きなフォントサイズでもキーが正しく生成されることを確認
    const int large_size{1024};
    const uint32_t codepoint{0x0041};  // 'A'
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(large_size, codepoint)};

    // サイズとコードポイントが正しく抽出できることを確認
    const int extracted_size{static_cast<int>(key >> TextRenderer::kCodepointBits)};
    const uint32_t extracted_codepoint{static_cast<uint32_t>(key & TextRenderer::kCodepointMask)};

    EXPECT_EQ(extracted_size, large_size);
    EXPECT_EQ(extracted_codepoint, codepoint);
}

TEST_F(TextRendererTest, MakeKey_EmojiCodepoint) {
    // 絵文字のコードポイントでキーを生成
    const int size_px{48};
    const uint32_t emoji_codepoint{0x1F6B4};  // 🚴 (自転車に乗る人)
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(size_px, emoji_codepoint)};

    // コードポイントとサイズが正しく抽出できることを確認
    const int extracted_size{static_cast<int>(key >> TextRenderer::kCodepointBits)};
    const uint32_t extracted_codepoint{static_cast<uint32_t>(key & TextRenderer::kCodepointMask)};

    EXPECT_EQ(extracted_size, size_px);
    EXPECT_EQ(extracted_codepoint, emoji_codepoint);
}

TEST_F(TextRendererTest, MakeKey_ZeroValues) {
    // サイズとコードポイントが0の場合
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(0, 0)};
    EXPECT_EQ(key, 0);
}

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

// =============================================================
// blitGlyphのユニットテスト
// -------------------------------------------------------------
// 要件：指定位置にグリフを正しく描画できること
// =============================================================
TEST_F(TextRendererTest, BlitGlyph_EmptyGlyph_WidthZero) {
    // 幅が0のグリフは描画されない
    TextRenderer::Glyph glyph;
    glyph.width = 0;
    glyph.height = 10;
    glyph.left = 0;
    glyph.top = 8;
    glyph.pitch = 0;

    // DrawRGB565Lineが呼ばれないことを期待
    EXPECT_CALL(mock_display, DrawRGB565Line(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);

    text_renderer.blitGlyph(100, 100, glyph);
}

TEST_F(TextRendererTest, BlitGlyph_EmptyGlyph_HeightZero) {
    // 高さが0のグリフは描画されない
    TextRenderer::Glyph glyph;
    glyph.width = 10;
    glyph.height = 0;
    glyph.left = 0;
    glyph.top = 8;
    glyph.pitch = 10;

    // DrawRGB565Lineが呼ばれないことを期待
    EXPECT_CALL(mock_display, DrawRGB565Line(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);

    text_renderer.blitGlyph(100, 100, glyph);
}

TEST_F(TextRendererTest, BlitGlyph_EmptyGlyph_NegativeWidth) {
    // 負の幅のグリフは描画されない
    TextRenderer::Glyph glyph;
    glyph.width = -5;
    glyph.height = 10;
    glyph.left = 0;
    glyph.top = 8;
    glyph.pitch = 0;

    // DrawRGB565Lineが呼ばれないことを期待
    EXPECT_CALL(mock_display, DrawRGB565Line(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);

    text_renderer.blitGlyph(100, 100, glyph);
}

TEST_F(TextRendererTest, BlitGlyph_EmptyGlyph_NegativeHeight) {
    // 負の高さのグリフは描画されない
    TextRenderer::Glyph glyph;
    glyph.width = 10;
    glyph.height = -5;
    glyph.left = 0;
    glyph.top = 8;
    glyph.pitch = 10;

    // DrawRGB565Lineが呼ばれないことを期待
    EXPECT_CALL(mock_display, DrawRGB565Line(::testing::_, ::testing::_, ::testing::_, ::testing::_)).Times(0);

    text_renderer.blitGlyph(100, 100, glyph);
}

TEST_F(TextRendererTest, BlitGlyph_ValidGlyph_DrawsCalls) {
    // 有効なグリフが指定された回数だけDrawRGB565Lineを呼ぶことを確認
    TextRenderer::Glyph glyph;
    glyph.width = 5;
    glyph.height = 3;
    glyph.left = 2;
    glyph.top = 8;
    glyph.pitch = 5;
    glyph.alpha.resize(glyph.height * glyph.pitch, 255);  // 完全不透明

    // 色を設定（前景：黒、背景：白）
    text_renderer.SetColors(Color565::Black(), Color565::White());

    const int baseline_x{100};
    const int baseline_y{100};
    const int expected_screen_x{baseline_x + glyph.left};  // 102
    const int expected_screen_y{baseline_y - glyph.top};   // 92

    // 各行（height=3）に対してDrawRGB565Lineが呼ばれる
    EXPECT_CALL(mock_display, DrawRGB565Line(expected_screen_x, expected_screen_y, ::testing::_, glyph.width)).Times(1);
    EXPECT_CALL(mock_display, DrawRGB565Line(expected_screen_x, expected_screen_y + 1, ::testing::_, glyph.width)).Times(1);
    EXPECT_CALL(mock_display, DrawRGB565Line(expected_screen_x, expected_screen_y + 2, ::testing::_, glyph.width)).Times(1);

    text_renderer.blitGlyph(baseline_x, baseline_y, glyph);
}

TEST_F(TextRendererTest, BlitGlyph_CorrectScreenCoordinates) {
    // スクリーン座標が正しく計算されることを確認
    TextRenderer::Glyph glyph;
    glyph.width = 8;
    glyph.height = 10;
    glyph.left = -2;  // 負のオフセット
    glyph.top = 12;   // 正のオフセット
    glyph.pitch = 8;
    glyph.alpha.resize(glyph.height * glyph.pitch, 128);  // 半透明

    const int baseline_x{50};
    const int baseline_y{200};
    const int expected_screen_x{baseline_x + glyph.left};  // 48
    const int expected_screen_y{baseline_y - glyph.top};   // 188

    // 各行（height=10）に対してDrawRGB565Lineが呼ばれることを確認
    for (int row{0}; row < glyph.height; ++row) {
        EXPECT_CALL(mock_display, DrawRGB565Line(expected_screen_x, expected_screen_y + row, ::testing::_, glyph.width)).Times(1);
    }

    text_renderer.blitGlyph(baseline_x, baseline_y, glyph);
}

TEST_F(TextRendererTest, BlitGlyph_AlphaBlending_FullyOpaque) {
    // 完全不透明なグリフが正しくブレンドされることを確認
    TextRenderer::Glyph glyph;
    glyph.width = 2;
    glyph.height = 1;
    glyph.left = 0;
    glyph.top = 0;
    glyph.pitch = 2;
    glyph.alpha = {255, 255};  // 完全不透明

    // 色を設定（前景：白、背景：黒）
    text_renderer.SetColors(Color565::White(), Color565::Black());

    const int baseline_x{0};
    const int baseline_y{0};

    // DrawRGB565Lineが呼ばれ、前景色（白）がそのまま描画される
    EXPECT_CALL(mock_display, DrawRGB565Line(::testing::_, ::testing::_, ::testing::NotNull(), glyph.width)).Times(1).WillOnce(::testing::Invoke([](int x, int y, const uint16_t *rgb565, int len) {
        // すべてのピクセルが白（0xFFFF）であることを確認
        for (int i{0}; i < len; ++i) {
            EXPECT_EQ(rgb565[i], Color565::White().value);
        }
    }));

    text_renderer.blitGlyph(baseline_x, baseline_y, glyph);
}

TEST_F(TextRendererTest, BlitGlyph_AlphaBlending_FullyTransparent) {
    // 完全透明なグリフが正しくブレンドされることを確認
    TextRenderer::Glyph glyph;
    glyph.width = 2;
    glyph.height = 1;
    glyph.left = 0;
    glyph.top = 0;
    glyph.pitch = 2;
    glyph.alpha = {0, 0};  // 完全透明

    // 色を設定（前景：白、背景：黒）
    text_renderer.SetColors(Color565::White(), Color565::Black());

    const int baseline_x{0};
    const int baseline_y{0};

    // DrawRGB565Lineが呼ばれ、背景色（黒）がそのまま描画される
    EXPECT_CALL(mock_display, DrawRGB565Line(::testing::_, ::testing::_, ::testing::NotNull(), glyph.width)).Times(1).WillOnce(::testing::Invoke([](int x, int y, const uint16_t *rgb565, int len) {
        // すべてのピクセルが黒（0x0000）であることを確認
        for (int i = 0; i < len; ++i) {
            EXPECT_EQ(rgb565[i], Color565::Black().value);
        }
    }));

    text_renderer.blitGlyph(baseline_x, baseline_y, glyph);
}

TEST_F(TextRendererTest, BlitGlyph_AlphaBlending_VariedAlpha) {
    // 異なるアルファ値を持つグリフが正しくブレンドされることを確認
    TextRenderer::Glyph glyph;
    glyph.width = 4;
    glyph.height = 1;
    glyph.left = 0;
    glyph.top = 0;
    glyph.pitch = 4;
    glyph.alpha = {0, 85, 170, 255};  // 0%, 33%, 67%, 100%の不透明度

    // 色を設定（前景：白、背景：黒）
    text_renderer.SetColors(Color565::White(), Color565::Black());

    const int baseline_x{0};
    const int baseline_y{0};

    // DrawRGB565Lineが呼ばれ、各ピクセルのアルファ値に応じてブレンドされる
    EXPECT_CALL(mock_display, DrawRGB565Line(::testing::_, ::testing::_, ::testing::NotNull(), glyph.width)).Times(1).WillOnce(::testing::Invoke([this](int x, int y, const uint16_t *rgb565, int len) {
        // 各ピクセルのブレンド結果を確認
        EXPECT_EQ(rgb565[0], text_renderer.Blend565(Color565::Black().value, Color565::White().value, 0));    // 背景色（黒）
        EXPECT_EQ(rgb565[1], text_renderer.Blend565(Color565::Black().value, Color565::White().value, 85));   // 約33%ブレンド
        EXPECT_EQ(rgb565[2], text_renderer.Blend565(Color565::Black().value, Color565::White().value, 170));  // 約67%ブレンド
        EXPECT_EQ(rgb565[3], text_renderer.Blend565(Color565::Black().value, Color565::White().value, 255));  // 前景色（白）
    }));

    text_renderer.blitGlyph(baseline_x, baseline_y, glyph);
}

TEST_F(TextRendererTest, BlitGlyph_MultipleRows) {
    // 複数行のグリフが正しく描画されることを確認
    TextRenderer::Glyph glyph;
    glyph.width = 3;
    glyph.height = 4;
    glyph.left = 1;
    glyph.top = 5;
    glyph.pitch = 3;
    // 各行に異なるアルファ値を設定
    glyph.alpha = {
        255, 255, 255,  // 1行目：完全不透明
        200, 200, 200,  // 2行目：約78%不透明
        128, 128, 128,  // 3行目：半透明
        50,  50,  50    // 4行目：ほぼ透明
    };

    // 色を設定（前景：赤、背景：青）
    const uint16_t red{0xF800};
    const uint16_t blue{0x001F};
    text_renderer.SetColors(Color565{red}, Color565{blue});

    const int baseline_x{10};
    const int baseline_y{20};
    const int expected_screen_x{baseline_x + glyph.left};
    const int expected_screen_y{baseline_y - glyph.top};

    // 各行ごとにDrawRGB565Lineが呼ばれることを確認
    for (int row{0}; row < glyph.height; ++row) {
        EXPECT_CALL(mock_display, DrawRGB565Line(expected_screen_x, expected_screen_y + row, ::testing::NotNull(), glyph.width))
            .Times(1)
            .WillOnce(::testing::Invoke([this, row, blue, red](int x, int y, const uint16_t *rgb565, int len) {
                // 各行のアルファ値に応じたブレンド結果を確認
                uint8_t expected_alpha{0};
                if (row == 0)
                    expected_alpha = 255;
                else if (row == 1)
                    expected_alpha = 200;
                else if (row == 2)
                    expected_alpha = 128;
                else if (row == 3)
                    expected_alpha = 50;

                const uint16_t expected_color{text_renderer.Blend565(blue, red, expected_alpha)};
                for (int i = 0; i < len; ++i) {
                    EXPECT_EQ(rgb565[i], expected_color);
                }
            }));
    }

    text_renderer.blitGlyph(baseline_x, baseline_y, glyph);
}

}  // namespace ui