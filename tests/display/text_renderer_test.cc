#include "display/text_renderer.h"

#include "display/impl/freetype_font_loader.h"
#include "mocks/display/mock_font_loader.h"
#include "mocks/driver/mock_display.h"

#include <gmock/gmock.h>
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

        // FreeTypeFontLoaderをインスタンス化
        font_loader = std::make_unique<FreeTypeFontLoader>(font_path);

        // TextRendererを初期化
        text_renderer = std::make_unique<TextRenderer>(mock_display, *font_loader);

        // フォントサイズのデフォルト設定
        text_renderer->SetFontSizePx(32);
    }

    void TearDown() override {
        // 各テスト実行後に呼ばれるクリーンアップ処理
        text_renderer.reset();
        font_loader.reset();
    }

    // テストで使用する共通のメンバ変数
    const std::string font_path{"/workspace/config/fonts/DejaVuSans.ttf"};
    driver::MockDisplay mock_display;
    std::unique_ptr<IFontLoader> font_loader;
    std::unique_ptr<TextRenderer> text_renderer;
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
    const uint16_t result{text_renderer->Blend565(background, foreground, TextRenderer::kAlphaMax)};
    EXPECT_EQ(result, foreground);
}

TEST_F(TextRendererTest, Blend565_FullyTransparent) {
    // アルファ = 0（完全透明）の場合、背景色がそのまま返る
    const uint16_t background{0xF800};  // 赤（RGB565）
    const uint16_t foreground{0x07E0};  // 緑（RGB565）
    const uint16_t result{text_renderer->Blend565(background, foreground, 0)};
    EXPECT_EQ(result, background);
}

TEST_F(TextRendererTest, Blend565_HalfTransparent) {
    // アルファ = 128（半透明）の場合、背景色と前景色が50:50で合成される
    const uint16_t background{0x0000};  // 黒（RGB565）
    const uint16_t foreground{0xFFFF};  // 白（RGB565）
    const uint16_t result{text_renderer->Blend565(background, foreground, TextRenderer::kAlphaMax / 2)};

    // 各色成分が約半分になることを確認
    const int result_red{text_renderer->ExtractColorComponent(result, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    const int result_green{text_renderer->ExtractColorComponent(result, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    const int result_blue{text_renderer->ExtractColorComponent(result, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};

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
    const uint16_t result{text_renderer->Blend565(black, white, TextRenderer::kAlphaMax / 4)};

    const int result_red{text_renderer->ExtractColorComponent(result, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    const int result_green{text_renderer->ExtractColorComponent(result, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    const int result_blue{text_renderer->ExtractColorComponent(result, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};

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
    const uint16_t red_green{text_renderer->Blend565(red, green, TextRenderer::kAlphaMax / 2)};
    const int rg_red{text_renderer->ExtractColorComponent(red_green, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    const int rg_green{text_renderer->ExtractColorComponent(red_green, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    const int rg_blue{text_renderer->ExtractColorComponent(red_green, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};

    EXPECT_EQ(rg_red, ((0xF800 >> TextRenderer::kRedShift) & TextRenderer::kRedMask) / 2);        // 赤成分の半分
    EXPECT_EQ(rg_green, ((0x07E0 >> TextRenderer::kGreenShift) & TextRenderer::kGreenMask) / 2);  // 緑成分の半分
    EXPECT_EQ(rg_blue, 0);                                                                        // 青成分はゼロ

    // 緑と青を50:50でブレンド → シアン系
    const uint16_t green_blue{text_renderer->Blend565(green, blue, TextRenderer::kAlphaMax / 2)};
    const int gb_red{text_renderer->ExtractColorComponent(green_blue, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    const int gb_green{text_renderer->ExtractColorComponent(green_blue, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    const int gb_blue{text_renderer->ExtractColorComponent(green_blue, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};

    EXPECT_EQ(gb_red, 0);                                                                         // 赤成分はゼロ
    EXPECT_EQ(gb_green, ((0x07E0 >> TextRenderer::kGreenShift) & TextRenderer::kGreenMask) / 2);  // 緑成分の半分
    EXPECT_EQ(gb_blue, ((0x001F >> TextRenderer::kBlueShift) & TextRenderer::kBlueMask) / 2);     // 青成分の半分

    // 青と赤を75:25でブレンド → 紫系
    const uint16_t blue_red{text_renderer->Blend565(blue, red, TextRenderer::kAlphaMax / 4)};
    const int br_red{text_renderer->ExtractColorComponent(blue_red, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    const int br_green{text_renderer->ExtractColorComponent(blue_red, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    const int br_blue{text_renderer->ExtractColorComponent(blue_red, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};

    EXPECT_EQ(br_red, ((0xF800 >> TextRenderer::kRedShift) & TextRenderer::kRedMask) / 4);         // 赤成分の25%
    EXPECT_EQ(br_green, 0);                                                                        // 緑成分はゼロ
    EXPECT_EQ(br_blue, ((0x001F >> TextRenderer::kBlueShift) & TextRenderer::kBlueMask) * 3 / 4);  // 青成分の75%
}

TEST_F(TextRendererTest, Blend565_SameColor) {
    // 背景色と前景色が同じ場合、アルファ値に関わらず同じ色が返る
    const uint16_t color{0x07E0};  // 緑（RGB565）
    const uint16_t result_opaque{text_renderer->Blend565(color, color, TextRenderer::kAlphaMax)};
    const uint16_t result_transparent{text_renderer->Blend565(color, color, 0)};
    const uint16_t result_half{text_renderer->Blend565(color, color, TextRenderer::kAlphaMax / 2)};

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
    const int r{text_renderer->ExtractColorComponent(color, TextRenderer::kRedShift, TextRenderer::kRedMask)};
    // 緑成分: 0x1E (0b011110)
    // 5ビットずらして，0x3F (0b111111)でマスク
    const int g{text_renderer->ExtractColorComponent(color, TextRenderer::kGreenShift, TextRenderer::kGreenMask)};
    // 青成分: 0x0D (0b01101)
    // 0ビットずらして，0x1F (0b11111)でマスク
    const int b{text_renderer->ExtractColorComponent(color, TextRenderer::kBlueShift, TextRenderer::kBlueMask)};
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

    text_renderer->blitGlyph(100, 100, glyph);
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

    text_renderer->blitGlyph(100, 100, glyph);
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

    text_renderer->blitGlyph(100, 100, glyph);
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

    text_renderer->blitGlyph(100, 100, glyph);
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
    text_renderer->SetColors(Color565::Black(), Color565::White());

    const int baseline_x{100};
    const int baseline_y{100};
    const int expected_screen_x{baseline_x + glyph.left};  // 102
    const int expected_screen_y{baseline_y - glyph.top};   // 92

    // 各行（height=3）に対してDrawRGB565Lineが呼ばれる
    EXPECT_CALL(mock_display, DrawRGB565Line(expected_screen_x, expected_screen_y, ::testing::_, glyph.width)).Times(1);
    EXPECT_CALL(mock_display, DrawRGB565Line(expected_screen_x, expected_screen_y + 1, ::testing::_, glyph.width)).Times(1);
    EXPECT_CALL(mock_display, DrawRGB565Line(expected_screen_x, expected_screen_y + 2, ::testing::_, glyph.width)).Times(1);

    text_renderer->blitGlyph(baseline_x, baseline_y, glyph);
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

    text_renderer->blitGlyph(baseline_x, baseline_y, glyph);
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
    text_renderer->SetColors(Color565::White(), Color565::Black());

    const int baseline_x{0};
    const int baseline_y{0};

    // DrawRGB565Lineが呼ばれ、前景色（白）がそのまま描画される
    EXPECT_CALL(mock_display, DrawRGB565Line(::testing::_, ::testing::_, ::testing::NotNull(), glyph.width)).Times(1).WillOnce(::testing::Invoke([](int x, int y, const uint16_t *rgb565, int len) {
        // すべてのピクセルが白（0xFFFF）であることを確認
        for (int i{0}; i < len; ++i) {
            EXPECT_EQ(rgb565[i], Color565::White().value);
        }
    }));

    text_renderer->blitGlyph(baseline_x, baseline_y, glyph);
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
    text_renderer->SetColors(Color565::White(), Color565::Black());

    const int baseline_x{0};
    const int baseline_y{0};

    // DrawRGB565Lineが呼ばれ、背景色（黒）がそのまま描画される
    EXPECT_CALL(mock_display, DrawRGB565Line(::testing::_, ::testing::_, ::testing::NotNull(), glyph.width)).Times(1).WillOnce(::testing::Invoke([](int x, int y, const uint16_t *rgb565, int len) {
        // すべてのピクセルが黒（0x0000）であることを確認
        for (int i = 0; i < len; ++i) {
            EXPECT_EQ(rgb565[i], Color565::Black().value);
        }
    }));

    text_renderer->blitGlyph(baseline_x, baseline_y, glyph);
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
    text_renderer->SetColors(Color565::White(), Color565::Black());

    const int baseline_x{0};
    const int baseline_y{0};

    // DrawRGB565Lineが呼ばれ、各ピクセルのアルファ値に応じてブレンドされる
    EXPECT_CALL(mock_display, DrawRGB565Line(::testing::_, ::testing::_, ::testing::NotNull(), glyph.width)).Times(1).WillOnce(::testing::Invoke([this](int x, int y, const uint16_t *rgb565, int len) {
        // 各ピクセルのブレンド結果を確認
        EXPECT_EQ(rgb565[0], text_renderer->Blend565(Color565::Black().value, Color565::White().value, 0));    // 背景色（黒）
        EXPECT_EQ(rgb565[1], text_renderer->Blend565(Color565::Black().value, Color565::White().value, 85));   // 約33%ブレンド
        EXPECT_EQ(rgb565[2], text_renderer->Blend565(Color565::Black().value, Color565::White().value, 170));  // 約67%ブレンド
        EXPECT_EQ(rgb565[3], text_renderer->Blend565(Color565::Black().value, Color565::White().value, 255));  // 前景色（白）
    }));

    text_renderer->blitGlyph(baseline_x, baseline_y, glyph);
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
    text_renderer->SetColors(Color565{red}, Color565{blue});

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

                const uint16_t expected_color{text_renderer->Blend565(blue, red, expected_alpha)};
                for (int i = 0; i < len; ++i) {
                    EXPECT_EQ(rgb565[i], expected_color);
                }
            }));
    }

    text_renderer->blitGlyph(baseline_x, baseline_y, glyph);
}

// =============================================================
// MeasureTextのユニットテスト
// -------------------------------------------------------------
// 要件：UTF-8文字列の描画に必要なメトリクス（幅・高さ・ベースライン）を正しく計測できること
// =============================================================
TEST_F(TextRendererTest, MeasureText_EmptyString) {
    // 空文字列の場合、幅は0になること
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText("")};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    EXPECT_EQ(metrics.width_px, 0);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_SingleLine_Ascii) {
    // 1行のASCII文字列
    const std::string text{"Hello"};
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // 幅：実際のグリフをロードして計測（概算式は使わない）
    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    // 幅は実際にロードされたグリフの合計幅を計算
    int expected_width = 0;
    size_t idx = 0;
    while (idx < text.size()) {
        uint32_t cp;
        if (TextRenderer::GetCodepoint(text, idx, cp)) {
            IFontLoader::GlyphData glyph_data;
            if (font_loader->LoadChar(cp, glyph_data) == 0) {
                expected_width += glyph_data.advance;
            }
        }
    }

    EXPECT_EQ(metrics.width_px, expected_width);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_SingleLine_Japanese) {
    // 1行の日本語文字列
    const std::string text{"こんにちは"};  // 5文字
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    // 幅は実際にロードされたグリフの合計幅を計算
    int expected_width = 0;
    size_t idx = 0;
    while (idx < text.size()) {
        uint32_t cp;
        if (TextRenderer::GetCodepoint(text, idx, cp)) {
            IFontLoader::GlyphData glyph_data;
            if (font_loader->LoadChar(cp, glyph_data) == 0) {
                expected_width += glyph_data.advance;
            }
        }
    }

    EXPECT_EQ(metrics.width_px, expected_width);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_MultipleLines_LastLineIsLongest) {
    // 複数行で最後の行が最も長い場合
    const std::string text{"Hi\nHello\nWorld!!"};
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    // 最長行（"World!!"）の幅を計算
    const std::string longest_line{"World!!"};
    int expected_width = 0;
    size_t idx = 0;
    while (idx < longest_line.size()) {
        uint32_t cp;
        if (TextRenderer::GetCodepoint(longest_line, idx, cp)) {
            IFontLoader::GlyphData glyph_data;
            if (font_loader->LoadChar(cp, glyph_data) == 0) {
                expected_width += glyph_data.advance;
            }
        }
    }

    EXPECT_EQ(metrics.width_px, expected_width);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_MultipleLines_MiddleLineIsLongest) {
    // 複数行で途中の行が最も長い場合
    const std::string text{"Hi\nHelloWorld\nOK"};
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    // 最長行（"HelloWorld"）の幅を計算
    const std::string longest_line{"HelloWorld"};
    int expected_width = 0;
    size_t idx = 0;
    while (idx < longest_line.size()) {
        uint32_t cp;
        if (TextRenderer::GetCodepoint(longest_line, idx, cp)) {
            IFontLoader::GlyphData glyph_data;
            if (font_loader->LoadChar(cp, glyph_data) == 0) {
                expected_width += glyph_data.advance;
            }
        }
    }

    EXPECT_EQ(metrics.width_px, expected_width);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_SingleNewline) {
    // 改行のみの文字列
    const std::string text{"\n"};
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    // 改行のみなので幅は0
    EXPECT_EQ(metrics.width_px, 0);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_MultipleNewlines) {
    // 複数の改行
    const std::string text{"\n\n\n"};
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    // 改行のみなので幅は0
    EXPECT_EQ(metrics.width_px, 0);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_TrailingNewline) {
    // 末尾に改行がある場合
    const std::string text{"Hello\n"};
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // 幅は実際にロードされたグリフの合計幅を計算
    const std::string text_without_newline{"Hello"};
    int expected_width = 0;
    size_t idx = 0;
    while (idx < text_without_newline.size()) {
        uint32_t cp;
        if (TextRenderer::GetCodepoint(text_without_newline, idx, cp)) {
            IFontLoader::GlyphData glyph_data;
            if (font_loader->LoadChar(cp, glyph_data) == 0) {
                expected_width += glyph_data.advance;
            }
        }
    }
    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    EXPECT_EQ(metrics.width_px, expected_width);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_MixedCharacters) {
    // ASCII、日本語、絵文字が混在する文字列
    const std::string text{"Hello世界🚴"};  // 5 + 2 + 1 = 8文字
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // 幅は実際にロードされたグリフの合計幅を計算
    int expected_width = 0;
    size_t idx = 0;
    while (idx < text.size()) {
        uint32_t cp;
        if (TextRenderer::GetCodepoint(text, idx, cp)) {
            IFontLoader::GlyphData glyph_data;
            if (font_loader->LoadChar(cp, glyph_data) == 0) {
                expected_width += glyph_data.advance;
            }
        }
    }
    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    EXPECT_EQ(metrics.width_px, expected_width);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_HeightIsLineHeight) {
    // 単一行の場合、高さは行高さと同じ
    const std::string text{"Test"};
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};

    // メトリクスの高さは FreeType の line_height と同じ
    EXPECT_EQ(metrics.height_px, expected_height);
}

TEST_F(TextRendererTest, MeasureText_BaselineIsAscent) {
    // ベースラインは ascent と同じ
    const std::string text{"Test"};
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_baseline{font_loader->GetAscentPx()};

    // ベースラインは ascent（文字の上端までの高さ）
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
    EXPECT_LE(metrics.baseline_px, metrics.height_px);  // 高さ以下であることも確認
}

TEST_F(TextRendererTest, MeasureText_DifferentFontSizes) {
    // フォントサイズを変更して幅が変わることを確認
    const std::string text{"Test"};

    text_renderer->SetFontSizePx(16);
    const TextRenderer::TextMetrics metrics_16{text_renderer->MeasureText(text)};

    text_renderer->SetFontSizePx(32);
    const TextRenderer::TextMetrics metrics_32{text_renderer->MeasureText(text)};

    text_renderer->SetFontSizePx(48);
    const TextRenderer::TextMetrics metrics_48{text_renderer->MeasureText(text)};

    // 幅がフォントサイズに応じて増加することを確認
    EXPECT_LT(metrics_16.width_px, metrics_32.width_px);
    EXPECT_LT(metrics_32.width_px, metrics_48.width_px);

    // 高さとベースラインがフォントサイズに応じて増加することを確認
    EXPECT_LT(metrics_16.height_px, metrics_32.height_px);
    EXPECT_LT(metrics_32.height_px, metrics_48.height_px);
    EXPECT_LT(metrics_16.baseline_px, metrics_32.baseline_px);
    EXPECT_LT(metrics_32.baseline_px, metrics_48.baseline_px);
}

TEST_F(TextRendererTest, MeasureText_InvalidUTF8_AtBeginning) {
    // 最初から不正なUTF-8シーケンス
    const std::string text{"\xF8\x80\x80\x80"};  // 無効な先頭バイト
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    // GetCodepointが失敗するため、breakで即座にループを抜ける
    // 結果として幅は0になる
    EXPECT_EQ(metrics.width_px, 0);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_InvalidUTF8_InMiddle) {
    // 正常な文字の後に不正なUTF-8シーケンス
    const std::string text{"AB\xF8\x80\x80\x80"};  // "AB" + 無効なバイト列
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    // "AB"（2文字）まで計測され、不正なシーケンスでbreakする
    const std::string valid_text{"AB"};
    int expected_width = 0;
    size_t idx = 0;
    while (idx < valid_text.size()) {
        uint32_t cp;
        if (TextRenderer::GetCodepoint(valid_text, idx, cp)) {
            IFontLoader::GlyphData glyph_data;
            if (font_loader->LoadChar(cp, glyph_data) == 0) {
                expected_width += glyph_data.advance;
            }
        }
    }
    EXPECT_EQ(metrics.width_px, expected_width);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_InvalidUTF8_IncompleteSequence) {
    // 不完全なUTF-8シーケンス（3バイト文字の1バイト目のみ）
    const std::string text{"Hello\xE3"};  // "Hello" + 3バイト文字の1バイト目のみ
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    // "Hello"（5文字）まで計測され、不完全なシーケンスでbreakする
    const std::string valid_text{"Hello"};
    int expected_width = 0;
    size_t idx = 0;
    while (idx < valid_text.size()) {
        uint32_t cp;
        if (TextRenderer::GetCodepoint(valid_text, idx, cp)) {
            IFontLoader::GlyphData glyph_data;
            if (font_loader->LoadChar(cp, glyph_data) == 0) {
                expected_width += glyph_data.advance;
            }
        }
    }
    EXPECT_EQ(metrics.width_px, expected_width);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

TEST_F(TextRendererTest, MeasureText_InvalidUTF8_AfterNewline) {
    // 改行の後に不正なUTF-8シーケンス
    const std::string text{
        "AB\nCD\xF8"
        "EF"};  // "AB" + 改行 + "CD" + 無効 + "EF"
    const TextRenderer::TextMetrics metrics{text_renderer->MeasureText(text)};

    // フォントローダーからメトリクスを取得
    const int expected_height{font_loader->GetLineHeightPx()};
    const int expected_baseline{font_loader->GetAscentPx()};

    // 1行目: "AB" = 2文字、2行目: "CD" = 2文字（不正なシーケンスでbreak）
    // 最大幅を計算
    auto calculate_width = [this](const std::string &str) {
        int width = 0;
        size_t idx = 0;
        while (idx < str.size()) {
            uint32_t cp;
            if (TextRenderer::GetCodepoint(str, idx, cp)) {
                IFontLoader::GlyphData glyph_data;
                if (font_loader->LoadChar(cp, glyph_data) == 0) {
                    width += glyph_data.advance;
                }
            }
        }
        return width;
    };
    const int width_line1 = calculate_width("AB");
    const int width_line2 = calculate_width("CD");
    const int expected_width = std::max(width_line1, width_line2);

    EXPECT_EQ(metrics.width_px, expected_width);
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_EQ(metrics.baseline_px, expected_baseline);
}

// =============================================================
// MeasureText フェイルセーフロジックの明示的テスト
// -------------------------------------------------------------
// 要件：line_height_px <= 0 の場合にフェイルセーフが確実に動作すること
// =============================================================
TEST_F(TextRendererTest, MeasureText_FailsafeLogic_ZeroLineHeight) {
    // MockFontLoaderを使用してline_height = 0を返すように設定
    MockFontLoader mock_font_loader;
    TextRenderer testable_renderer{mock_display, mock_font_loader};
    testable_renderer.SetFontSizePx(32);
    const int font_size = 32;
    const int line_gap = 4;  // TextRendererのデフォルト値

    // line_height_px = 0 を強制
    EXPECT_CALL(mock_font_loader, GetLineHeightPx()).WillRepeatedly(testing::Return(0));
    EXPECT_CALL(mock_font_loader, GetAscentPx()).WillRepeatedly(testing::Return(20));

    // LoadCharを設定（グリフロード時に呼ばれる）
    EXPECT_CALL(mock_font_loader, LoadChar(testing::_, testing::_)).WillRepeatedly(testing::Invoke([](uint32_t codepoint, IFontLoader::GlyphData &glyph_data) {
        glyph_data.width = 10;
        glyph_data.height = 15;
        glyph_data.advance = 12;
        glyph_data.left = 1;
        glyph_data.top = 14;
        glyph_data.pitch = 10;
        glyph_data.alpha.resize(150);
        return 0;
    }));

    const std::string text{"Test"};
    const TextRenderer::TextMetrics metrics{testable_renderer.MeasureText(text)};

    // フェイルセーフ値を計算: ascent + descent + line_gap_px_
    // descent ≈ font_size - ascent = 32 - 20 = 12
    const int expected_height{20 + 12 + line_gap};

    // フェイルセーフが発動し、計算された値が使用されることを確認
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_GT(metrics.height_px, 0);
}

TEST_F(TextRendererTest, MeasureText_FailsafeLogic_NegativeLineHeight) {
    // 負の値の場合もフェイルセーフが動作することを確認
    MockFontLoader mock_font_loader;
    TextRenderer testable_renderer{mock_display, mock_font_loader};
    testable_renderer.SetFontSizePx(32);
    const int font_size = 32;
    const int line_gap = 4;

    // line_height_px = -5 を強制
    EXPECT_CALL(mock_font_loader, GetLineHeightPx()).WillRepeatedly(testing::Return(-5));
    EXPECT_CALL(mock_font_loader, GetAscentPx()).WillRepeatedly(testing::Return(20));

    // LoadCharを設定
    EXPECT_CALL(mock_font_loader, LoadChar(testing::_, testing::_)).WillRepeatedly(testing::Invoke([](uint32_t codepoint, IFontLoader::GlyphData &glyph_data) {
        glyph_data.width = 10;
        glyph_data.height = 15;
        glyph_data.advance = 12;
        glyph_data.left = 1;
        glyph_data.top = 14;
        glyph_data.pitch = 10;
        glyph_data.alpha.resize(150);
        return 0;
    }));

    const std::string text{"Test"};
    const TextRenderer::TextMetrics metrics{testable_renderer.MeasureText(text)};

    // フェイルセーフ値を計算: ascent + descent + line_gap_px_
    const int expected_height{20 + 12 + line_gap};

    // フェイルセーフが発動することを確認
    EXPECT_EQ(metrics.height_px, expected_height);
    EXPECT_GT(metrics.height_px, 0);
}

// =============================================================
// getGlyph のユニットテスト
// -------------------------------------------------------------
// 要件：
// 1. キャッシュが存在しない場合：
//    - グリフがフォントローダーからロードされること
//    - ロードされたグリフがキャッシュに追加されること
//    - 追加されたグリフのアドレスが返されること
// 2. キャッシュが存在する場合：
//    - キャッシュから直接グリフが取得されること
//    - 同じアドレスが返されること（ロード処理が発生しないこと）
// =============================================================

// 要件1: キャッシュが存在しない場合、グリフがロードされてキャッシュに追加されること
// 1-1. 初回アクセス時にグリフがロードされてキャッシュに追加されること
TEST_F(TextRendererTest, GetGlyph_FirstAccess_LoadsAndCaches) {
    const uint32_t codepoint{0x0041};  // 'A'

    // キャッシュが空であることを確認
    EXPECT_EQ(text_renderer->cache_.empty(), true);

    // getGlyphを呼び出す
    const TextRenderer::Glyph *glyph{text_renderer->getGlyph(codepoint)};

    // グリフが返されることを確認
    EXPECT_NE(glyph, nullptr);

    // グリフの内容が有効であることを確認
    EXPECT_GT(glyph->width, 0);
    EXPECT_GT(glyph->height, 0);
    EXPECT_GT(glyph->advance, 0);

    // キャッシュにグリフが追加されたことを確認
    EXPECT_EQ(text_renderer->cache_.size(), 1);

    // キャッシュ内のグリフと返されたグリフが同じアドレスであることを確認
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(text_renderer->font_size_px_, codepoint)};
    EXPECT_EQ(glyph, &text_renderer->cache_[key]);
}

// 1-2. 異なるコードポイントで異なるグリフが返されること
TEST_F(TextRendererTest, GetGlyph_DifferentCodepoints_ReturnDifferentGlyphs) {
    const uint32_t codepoint_a{0x0041};  // 'A'
    const uint32_t codepoint_b{0x0042};  // 'B'

    // それぞれのグリフを取得
    const TextRenderer::Glyph *glyph_a{text_renderer->getGlyph(codepoint_a)};
    const TextRenderer::Glyph *glyph_b{text_renderer->getGlyph(codepoint_b)};

    // 異なるポインタが返されることを確認
    EXPECT_NE(glyph_a, glyph_b);

    // キャッシュに2つのグリフが追加されていることを確認
    EXPECT_GE(text_renderer->cache_.size(), 2);

    // グリフの内容も異なることを確認（少なくとも1つのメトリクスが異なる）
    const bool different{(glyph_a->width != glyph_b->width) || (glyph_a->height != glyph_b->height) || (glyph_a->advance != glyph_b->advance) || (glyph_a->alpha != glyph_b->alpha)};
    EXPECT_EQ(different, true);
}

// 1-3. 同じコードポイントでも異なるフォントサイズで異なるグリフが返されること
TEST_F(TextRendererTest, GetGlyph_DifferentFontSizes_ReturnDifferentGlyphs) {
    const uint32_t codepoint{0x0041};  // 'A'

    // フォントサイズ16でグリフを取得
    text_renderer->SetFontSizePx(16);
    const TextRenderer::Glyph *glyph_16{text_renderer->getGlyph(codepoint)};
    EXPECT_NE(glyph_16, nullptr);

    // フォントサイズ32でグリフを取得
    text_renderer->SetFontSizePx(32);
    const TextRenderer::Glyph *glyph_32{text_renderer->getGlyph(codepoint)};
    EXPECT_NE(glyph_32, nullptr);

    // 異なるポインタが返されることを確認
    EXPECT_NE(glyph_16, glyph_32);

    // キャッシュには2つのグリフが存在することを確認
    EXPECT_GE(text_renderer->cache_.size(), 2);

    // グリフのサイズが異なることを確認
    EXPECT_LT(glyph_16->width, glyph_32->width);
    EXPECT_LT(glyph_16->height, glyph_32->height);
    EXPECT_LT(glyph_16->advance, glyph_32->advance);
}

// 1-4. スペース文字もキャッシュされること
TEST_F(TextRendererTest, GetGlyph_SpaceCharacter_IsCached) {
    const uint32_t codepoint_space{0x0020};  // ' '

    // スペース文字のグリフを取得
    const TextRenderer::Glyph *glyph{text_renderer->getGlyph(codepoint_space)};
    EXPECT_NE(glyph, nullptr);

    // キャッシュに追加されていることを確認
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(text_renderer->font_size_px_, codepoint_space)};
    EXPECT_EQ(text_renderer->cache_.count(key), 1);

    // 2回目のアクセスで同じポインタが返されることを確認
    const TextRenderer::Glyph *glyph2{text_renderer->getGlyph(codepoint_space)};
    EXPECT_EQ(glyph, glyph2);
}

// 1-5. 絵文字もキャッシュされること
TEST_F(TextRendererTest, GetGlyph_EmojiCharacter_IsCached) {
    const uint32_t codepoint_emoji{0x1F6B4};  // '🚴'

    // 絵文字のグリフを取得
    const TextRenderer::Glyph *glyph{text_renderer->getGlyph(codepoint_emoji)};
    EXPECT_NE(glyph, nullptr);

    // キャッシュに追加されていることを確認
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(text_renderer->font_size_px_, codepoint_emoji)};
    EXPECT_EQ(text_renderer->cache_.count(key), 1);

    // 2回目のアクセスで同じポインタが返されることを確認
    const TextRenderer::Glyph *glyph2{text_renderer->getGlyph(codepoint_emoji)};
    EXPECT_EQ(glyph, glyph2);
}

// 1-6. 複数の文字をキャッシュした後、各文字に正しくアクセスできること
TEST_F(TextRendererTest, GetGlyph_MultipleCharacters_AllCached) {
    // 複数の文字をキャッシュ
    const std::vector<uint32_t> codepoints{0x0041, 0x0042, 0x0043, 0x3042, 0x3044, 0x3046};  // A, B, C, あ, い, う

    // すべての文字のグリフを取得してキャッシュ
    std::vector<const TextRenderer::Glyph *> glyphs;
    for (const uint32_t cp : codepoints) {
        const TextRenderer::Glyph *glyph{text_renderer->getGlyph(cp)};
        EXPECT_NE(glyph, nullptr);
        glyphs.push_back(glyph);
    }

    // キャッシュサイズを確認
    EXPECT_GE(text_renderer->cache_.size(), codepoints.size());

    // 再度アクセスして、同じポインタが返されることを確認
    for (size_t i{0}; i < codepoints.size(); ++i) {
        const TextRenderer::Glyph *glyph{text_renderer->getGlyph(codepoints[i])};
        EXPECT_EQ(glyph, glyphs[i]);
    }
}

// 1-7. 存在しないコードポイントでもnullptrではなくグリフが返されること
TEST_F(TextRendererTest, GetGlyph_InvalidCodepoint_ReturnsValidGlyph) {
    const uint32_t invalid_codepoint{0xFFFFFFFF};  // 存在しないコードポイント

    // グリフを取得
    const TextRenderer::Glyph *glyph{text_renderer->getGlyph(invalid_codepoint)};

    // nullptrではなく有効なポインタが返されることを確認
    EXPECT_NE(glyph, nullptr);

    // デフォルトグリフが返されることを確認（幅・高さが0より大きい）
    EXPECT_GT(glyph->width, 0);
    EXPECT_GT(glyph->height, 0);

    // キャッシュに追加されていることを確認
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(text_renderer->font_size_px_, invalid_codepoint)};
    EXPECT_EQ(text_renderer->cache_.count(key), 1);
}

// 要件2: キャッシュが存在する場合、キャッシュから取得されること
// 2-1. 2回目以降のアクセス時にキャッシュから取得されること（同じポインタが返ること）
TEST_F(TextRendererTest, GetGlyph_SecondAccess_ReturnsCachedGlyph) {
    const uint32_t codepoint{0x0042};  // 'B'

    // 1回目のアクセス
    const TextRenderer::Glyph *glyph1{text_renderer->getGlyph(codepoint)};
    EXPECT_NE(glyph1, nullptr);

    // キャッシュサイズを記録
    const size_t cache_size_after_first{text_renderer->cache_.size()};

    // 2回目のアクセス
    const TextRenderer::Glyph *glyph2{text_renderer->getGlyph(codepoint)};
    EXPECT_NE(glyph2, nullptr);

    // 同じポインタが返されることを確認
    EXPECT_EQ(glyph1, glyph2);

    // キャッシュサイズが変わっていないことを確認
    EXPECT_EQ(text_renderer->cache_.size(), cache_size_after_first);

    // グリフの内容が同じであることを確認
    EXPECT_EQ(glyph1->width, glyph2->width);
    EXPECT_EQ(glyph1->height, glyph2->height);
    EXPECT_EQ(glyph1->advance, glyph2->advance);
}

// 2-2. 複数回アクセスしても常に同じポインタが返されること
TEST_F(TextRendererTest, GetGlyph_MultipleAccesses_ReturnsSamePointer) {
    const uint32_t codepoint{0x3042};  // 'あ'

    // 複数回アクセス
    const TextRenderer::Glyph *glyph1{text_renderer->getGlyph(codepoint)};
    const TextRenderer::Glyph *glyph2{text_renderer->getGlyph(codepoint)};
    const TextRenderer::Glyph *glyph3{text_renderer->getGlyph(codepoint)};
    const TextRenderer::Glyph *glyph4{text_renderer->getGlyph(codepoint)};

    // すべて同じポインタであることを確認
    EXPECT_EQ(glyph1, glyph2);
    EXPECT_EQ(glyph2, glyph3);
    EXPECT_EQ(glyph3, glyph4);

    // キャッシュには1つだけ追加されていることを確認
    const TextRenderer::GlyphKey key{TextRenderer::MakeKey(text_renderer->font_size_px_, codepoint)};
    EXPECT_EQ(text_renderer->cache_.count(key), 1);
}

// =============================================================
// loadGlyph のユニットテスト
// -------------------------------------------------------------
// 要件：
// 1. フォントローダーからグリフデータを正しくロードできること
// 2. ロードしたグリフデータを内部のGlyph構造体に正しく変換できること
// 3. LoadCharが失敗した場合、初期化されたグリフ（ゼロ値）を返すこと
// =============================================================

// 要件1&2: グリフデータが正しくロードされ、Glyph構造体に正しく変換されること
// 1-1. ASCII文字のグリフが正しくロードされること
TEST_F(TextRendererTest, LoadGlyph_AsciiCharacter) {
    // 'A' (U+0041) をロード
    const uint32_t codepoint_a{0x0041};
    const TextRenderer::Glyph glyph_a{text_renderer->loadGlyph(codepoint_a)};

    // グリフの基本的な情報が正しく設定されていることを確認
    EXPECT_GT(glyph_a.width, 0);                                      // 幅が正の値
    EXPECT_GT(glyph_a.height, 0);                                     // 高さが正の値
    EXPECT_GT(glyph_a.advance, 0);                                    // アドバンス値が正の値
    EXPECT_GE(glyph_a.pitch, glyph_a.width);                          // pitchは幅以上
    EXPECT_EQ(glyph_a.alpha.size(), glyph_a.height * glyph_a.pitch);  // アルファデータのサイズが正しい
}

// 1-2. 日本語文字のグリフが正しくロードされること
TEST_F(TextRendererTest, LoadGlyph_JapaneseCharacter) {
    // 'あ' (U+3042) をロード
    const uint32_t codepoint_hiragana{0x3042};
    const TextRenderer::Glyph glyph_hiragana{text_renderer->loadGlyph(codepoint_hiragana)};

    // グリフの基本的な情報が正しく設定されていることを確認
    EXPECT_GT(glyph_hiragana.width, 0);
    EXPECT_GT(glyph_hiragana.height, 0);
    EXPECT_GT(glyph_hiragana.advance, 0);
    EXPECT_GE(glyph_hiragana.pitch, glyph_hiragana.width);
    EXPECT_EQ(glyph_hiragana.alpha.size(), glyph_hiragana.height * glyph_hiragana.pitch);
}

// 1-3. スペース文字のグリフが正しく処理されること
TEST_F(TextRendererTest, LoadGlyph_SpaceCharacter) {
    // ' ' (U+0020) をロード
    const uint32_t codepoint_space{0x0020};
    const TextRenderer::Glyph glyph_space{text_renderer->loadGlyph(codepoint_space)};

    // スペースはアドバンス値を持つが、ビットマップデータは持たない場合がある
    EXPECT_GT(glyph_space.advance, 0);  // アドバンス値は正の値

    // ビットマップがない場合、幅と高さは0
    if (glyph_space.width == 0 || glyph_space.height == 0) {
        EXPECT_TRUE(glyph_space.alpha.empty());  // アルファデータも空
    }
}

// 1-4. 絵文字のグリフがロードされること（フォントに存在する場合）
TEST_F(TextRendererTest, LoadGlyph_EmojiCharacter) {
    // '🚴' (U+1F6B4) をロード
    const uint32_t codepoint_emoji{0x1F6B4};
    const TextRenderer::Glyph glyph_emoji{text_renderer->loadGlyph(codepoint_emoji)};

    // フォントに絵文字が含まれていない場合はデフォルトグリフが返される
    // いずれにしてもグリフは返される
    EXPECT_GT(glyph_emoji.width, 0);
    EXPECT_GT(glyph_emoji.height, 0);
    EXPECT_FALSE(glyph_emoji.alpha.empty());
}

// 1-5. 存在しないコードポイントでもデフォルトグリフが返されること
TEST_F(TextRendererTest, LoadGlyph_InvalidCodepointReturnsDefaultGlyph) {
    // 注意：FreeTypeは存在しないコードポイントに対してもデフォルトグリフ（.notdef）を返す
    // そのため、FT_Load_Charは失敗せず、何らかのグリフが返される
    const uint32_t invalid_codepoint{0xFFFFFFFF};  // 存在しないコードポイント
    const TextRenderer::Glyph glyph{text_renderer->loadGlyph(invalid_codepoint)};

    // デフォルトグリフが返されることを確認（幅・高さが0より大きい）
    EXPECT_GT(glyph.width, 0);
    EXPECT_GT(glyph.height, 0);
    EXPECT_EQ(glyph.pitch, glyph.width);                        // 通常、pitchは幅と同じ
    EXPECT_FALSE(glyph.alpha.empty());                          // アルファデータが存在
    EXPECT_EQ(glyph.alpha.size(), glyph.height * glyph.pitch);  // サイズが正しい
}

// 1-6. 異なる文字で異なるグリフが返されること
TEST_F(TextRendererTest, LoadGlyph_DifferentCharactersReturnDifferentGlyphs) {
    // 'A' と 'B' をロード
    const TextRenderer::Glyph glyph_a{text_renderer->loadGlyph(0x0041)};
    const TextRenderer::Glyph glyph_b{text_renderer->loadGlyph(0x0042)};

    // 異なる文字なので、何らかのメトリクスが異なるはず
    // （幅、高さ、アドバンス値、またはビットマップデータのいずれか）
    const bool different{(glyph_a.width != glyph_b.width) || (glyph_a.height != glyph_b.height) || (glyph_a.advance != glyph_b.advance) || (glyph_a.alpha != glyph_b.alpha)};
    EXPECT_TRUE(different);
}

// 1-7. フォントサイズを変更すると異なるグリフが返されること
TEST_F(TextRendererTest, LoadGlyph_DifferentFontSizes) {
    // フォントサイズ16で 'A' をロード
    text_renderer->SetFontSizePx(16);
    const TextRenderer::Glyph glyph_16{text_renderer->loadGlyph(0x0041)};

    // フォントサイズ32で 'A' をロード
    text_renderer->SetFontSizePx(32);
    const TextRenderer::Glyph glyph_32{text_renderer->loadGlyph(0x0041)};

    // フォントサイズが異なるので、グリフのサイズも異なるはず
    EXPECT_LT(glyph_16.width, glyph_32.width);
    EXPECT_LT(glyph_16.height, glyph_32.height);
    EXPECT_LT(glyph_16.advance, glyph_32.advance);
}

// 要件3: LoadCharが失敗した場合、初期化されたグリフ（ゼロ値）が返されること
// 3-1. LoadCharの失敗時にゼロ値のグリフが返されること
TEST_F(TextRendererTest, LoadGlyph_FT_Load_Char_Failure) {
    // MockFontLoaderを使用してLoadCharの失敗をシミュレート
    MockFontLoader mock_font_loader;
    TextRenderer testable_renderer{mock_display, mock_font_loader};
    testable_renderer.SetFontSizePx(32);
    const uint32_t codepoint{0x0041};  // 'A' のコードポイント

    // LoadCharが失敗（非ゼロの戻り値）を返すように設定
    EXPECT_CALL(mock_font_loader, LoadChar(codepoint, testing::_)).WillOnce(testing::Return(1));  // FT_Load_Charは失敗時に非ゼロを返す

    const TextRenderer::Glyph glyph{testable_renderer.loadGlyph(codepoint)};

    // グリフの初期値が返されることを確認
    EXPECT_EQ(glyph.width, 0);
    EXPECT_EQ(glyph.height, 0);
    EXPECT_EQ(glyph.left, 0);
    EXPECT_EQ(glyph.top, 0);
    EXPECT_EQ(glyph.advance, 0);
    EXPECT_EQ(glyph.pitch, 0);
    EXPECT_TRUE(glyph.alpha.empty());
}

// =============================================================
// SetWrapWidthPxのユニットテスト
// -------------------------------------------------------------
// 要件：wrap_width_px_に0以上の任意の値を設定できること．
// 1. 0以上の値を設定した場合、その値がwrap_width_px_に正しく設定されること
// 2. 0未満の値を設定した場合，0がwrap_width_px_に設定されること
// (理由) 0未満は意図しない値のため，デフォルト値を設定する
// =============================================================

// 要件1: 0以上の値を設定した場合、その値がwrap_width_px_に正しく設定されること
TEST_F(TextRendererTest, SetWrapWidthPx_PositiveValue) {
    text_renderer->SetWrapWidthPx(480);
    EXPECT_EQ(text_renderer->wrap_width_px_, 480);
}

TEST_F(TextRendererTest, SetWrapWidthPx_ZeroValue) {
    text_renderer->SetWrapWidthPx(0);
    EXPECT_EQ(text_renderer->wrap_width_px_, 0);
}

// 要件2: 0未満の値を設定した場合，0がwrap_width_px_に設定されること
TEST_F(TextRendererTest, SetWrapWidthPx_NegativeValue) {
    text_renderer->SetWrapWidthPx(-100);
    EXPECT_EQ(text_renderer->wrap_width_px_, 0);
}

// =============================================================
// SetLineGapPxのユニットテスト
// -------------------------------------------------------------
// 要件：line_gap_px_に0以上の任意の値を設定できること．
// 1. 0以上の値を設定した場合、その値がline_gap_px_に正しく設定されること
// 2. 0未満の値を設定した場合，0がline_gap_px_に設定されること
// (理由) 0未満は意図しない値のため，デフォルト値を設定する
// =============================================================

// 要件1: 0以上の値を設定した場合、その値がline_gap_px_に正しく設定されること
TEST_F(TextRendererTest, SetLineGapPx_PositiveValue) {
    text_renderer->SetLineGapPx(10);
    EXPECT_EQ(text_renderer->line_gap_px_, 10);
}

TEST_F(TextRendererTest, SetLineGapPx_ZeroValue) {
    text_renderer->SetLineGapPx(0);
    EXPECT_EQ(text_renderer->line_gap_px_, 0);
}

// 要件2: 0未満の値を設定した場合，0がline_gap_px_に設定されること
TEST_F(TextRendererTest, SetLineGapPx_NegativeValue) {
    text_renderer->SetLineGapPx(-5);
    EXPECT_EQ(text_renderer->line_gap_px_, 0);
}

// =============================================================
// SetFontSizePxのユニットテスト
// -------------------------------------------------------------
// 要件：font_size_px_にkMinFontSizePx以上の任意の値を設定できること．
// 1. kMinFontSizePx以上の値を設定した場合、その値がfont_size_px_に正しく設定されること
// 2. kMinFontSizePx未満の値を設定した場合，kMinFontSizePxがfont_size_px_に設定されること
// (理由) kMinFontSizePx未満は視認性の問題があるため，最小値を設定する
// =============================================================

// 要件1: kMinFontSizePx以上の値を設定した場合、その値がfont_size_px_に正しく設定されること
TEST_F(TextRendererTest, SetFontSizePx_ValidValue) {
    const int font_size_px{TextRenderer::kMinFontSizePx + 10};
    text_renderer->SetFontSizePx(font_size_px);
    EXPECT_EQ(text_renderer->font_size_px_, font_size_px);
}

TEST_F(TextRendererTest, SetFontSizePx_MinValue) {
    const int font_size_px{TextRenderer::kMinFontSizePx};
    text_renderer->SetFontSizePx(font_size_px);
    EXPECT_EQ(text_renderer->font_size_px_, font_size_px);
}

// 要件2: kMinFontSizePx未満の値を設定した場合，kMinFontSizePxがfont_size_px_に設定されること
TEST_F(TextRendererTest, SetFontSizePx_BelowMinValue) {
    const int font_size_px{TextRenderer::kMinFontSizePx - 1};
    text_renderer->SetFontSizePx(font_size_px);
    EXPECT_EQ(text_renderer->font_size_px_, TextRenderer::kMinFontSizePx);
}

}  // namespace ui