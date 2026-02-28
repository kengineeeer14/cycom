#ifndef CYCOM_DISPLAY_TEXT_RENDERER_H_
#define CYCOM_DISPLAY_TEXT_RENDERER_H_

#include "display/color.h"
#include "display/interface/i_font_loader.h"
#include "driver/interface/i_display.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui {

class TextRenderer {
    // テスト用フレンドクラス
    friend class TextRendererTest;
    friend class TextRendererTest_MakeKey_BasicGeneration_Test;
    friend class TextRendererTest_MakeKey_DifferentSizes_Test;
    friend class TextRendererTest_MakeKey_DifferentCodepoints_Test;
    friend class TextRendererTest_MakeKey_MaxCodepoint_Test;
    friend class TextRendererTest_MakeKey_CodepointMasking_Test;
    friend class TextRendererTest_MakeKey_LargeSize_Test;
    friend class TextRendererTest_MakeKey_EmojiCodepoint_Test;
    friend class TextRendererTest_MakeKey_ZeroValues_Test;
    friend class TextRendererTest_Blend565_FullyOpaque_Test;
    friend class TextRendererTest_Blend565_FullyTransparent_Test;
    friend class TextRendererTest_Blend565_HalfTransparent_Test;
    friend class TextRendererTest_Blend565_MaxColorComponents_Test;
    friend class TextRendererTest_Blend565_PrimaryColors_Test;
    friend class TextRendererTest_Blend565_SameColor_Test;
    friend class TextRendererTest_ExtractColorComponentTest_Test;
    friend class TextRendererTest_GetCodepoint_AsciiCharacter_Test;
    friend class TextRendererTest_GetCodepoint_TwoByteCharacter_Test;
    friend class TextRendererTest_GetCodepoint_ThreeByteCharacter_Test;
    friend class TextRendererTest_GetCodepoint_FourByteCharacter_Test;
    friend class TextRendererTest_GetCodepoint_MixedCharacters_Test;
    friend class TextRendererTest_GetCodepoint_EndOfString_Test;
    friend class TextRendererTest_GetCodepoint_IncompleteTwoByteSequence_Test;
    friend class TextRendererTest_GetCodepoint_IncompleteThreeByteSequence_Test;
    friend class TextRendererTest_GetCodepoint_IncompleteFourByteSequence_Test;
    friend class TextRendererTest_GetCodepoint_InvalidSequence_Test;
    friend class TextRendererTest_GetCodepoint_EmptyString_Test;
    friend class TextRendererTest_GetCodepoint_SequentialCalls_Test;
    friend class TextRendererTest_BlitGlyph_EmptyGlyph_WidthZero_Test;
    friend class TextRendererTest_BlitGlyph_EmptyGlyph_HeightZero_Test;
    friend class TextRendererTest_BlitGlyph_EmptyGlyph_NegativeWidth_Test;
    friend class TextRendererTest_BlitGlyph_EmptyGlyph_NegativeHeight_Test;
    friend class TextRendererTest_BlitGlyph_ValidGlyph_DrawsCalls_Test;
    friend class TextRendererTest_BlitGlyph_CorrectScreenCoordinates_Test;
    friend class TextRendererTest_BlitGlyph_AlphaBlending_FullyOpaque_Test;
    friend class TextRendererTest_BlitGlyph_AlphaBlending_FullyTransparent_Test;
    friend class TextRendererTest_BlitGlyph_AlphaBlending_VariedAlpha_Test;
    friend class TextRendererTest_BlitGlyph_MultipleRows_Test;
    friend class TextRendererTest_MeasureText_EmptyString_Test;
    friend class TextRendererTest_MeasureText_SingleLine_Ascii_Test;
    friend class TextRendererTest_MeasureText_SingleLine_Japanese_Test;
    friend class TextRendererTest_MeasureText_MultipleLines_LastLineIsLongest_Test;
    friend class TextRendererTest_MeasureText_MultipleLines_MiddleLineIsLongest_Test;
    friend class TextRendererTest_MeasureText_SingleNewline_Test;
    friend class TextRendererTest_MeasureText_MultipleNewlines_Test;
    friend class TextRendererTest_MeasureText_TrailingNewline_Test;
    friend class TextRendererTest_MeasureText_MixedCharacters_Test;
    friend class TextRendererTest_MeasureText_HeightIsLineHeight_Test;
    friend class TextRendererTest_MeasureText_BaselineIsAscent_Test;
    friend class TextRendererTest_MeasureText_DifferentFontSizes_Test;
    friend class TextRendererTest_MeasureText_InvalidUTF8_AtBeginning_Test;
    friend class TextRendererTest_MeasureText_InvalidUTF8_InMiddle_Test;
    friend class TextRendererTest_MeasureText_InvalidUTF8_IncompleteSequence_Test;
    friend class TextRendererTest_MeasureText_InvalidUTF8_AfterNewline_Test;
    friend class TextRendererTest_MeasureText_FailsafeLogic_ZeroLineHeight_Test;
    friend class TextRendererTest_MeasureText_FailsafeLogic_NegativeLineHeight_Test;
    friend class TextRendererTest_LoadGlyph_InvalidCodepointReturnsDefaultGlyph_Test;
    friend class TextRendererTest_LoadGlyph_AsciiCharacter_Test;
    friend class TextRendererTest_LoadGlyph_JapaneseCharacter_Test;
    friend class TextRendererTest_LoadGlyph_SpaceCharacter_Test;
    friend class TextRendererTest_LoadGlyph_EmojiCharacter_Test;
    friend class TextRendererTest_LoadGlyph_DifferentCharactersReturnDifferentGlyphs_Test;
    friend class TextRendererTest_LoadGlyph_DifferentFontSizes_Test;
    friend class TextRendererTest_LoadGlyph_AlphaDataIsIndependent_Test;
    friend class TextRendererTest_LoadGlyph_MetricsAreValid_Test;
    friend class TextRendererTest_LoadGlyph_AlphaSizeMatchesBitmap_Test;
    friend class TextRendererTest_LoadGlyph_FT_Load_Char_Failure_Test;
    friend class TextRendererTest_GetGlyph_FirstAccess_LoadsAndCaches_Test;
    friend class TextRendererTest_GetGlyph_SecondAccess_ReturnsCachedGlyph_Test;
    friend class TextRendererTest_GetGlyph_MultipleAccesses_ReturnsSamePointer_Test;
    friend class TextRendererTest_GetGlyph_DifferentCodepoints_ReturnDifferentGlyphs_Test;
    friend class TextRendererTest_GetGlyph_DifferentFontSizes_ReturnDifferentGlyphs_Test;
    friend class TextRendererTest_GetGlyph_SpaceCharacter_IsCached_Test;
    friend class TextRendererTest_GetGlyph_EmojiCharacter_IsCached_Test;
    friend class TextRendererTest_GetGlyph_MultipleCharacters_AllCached_Test;
    friend class TextRendererTest_GetGlyph_InvalidCodepoint_ReturnsValidGlyph_Test;

  public:
    // 型・エイリアス
    struct TextMetrics {
        int width_px;     // テキスト全体の中で最も幅が広い行の幅
        int height_px;    // 1行分の推奨される総高さです。次の行までの距離で、ascent + descent + 行間を含む．
        int baseline_px;  // // ベースラインから文字上端までの高さ．大文字や上に伸びる文字（'A', 'h', 'b'など）の高さ．
    };

    // コンストラクタ/デストラクタ
    // font_loader: フォントローディングを行う実装（FreeTypeFontLoaderまたはMockFontLoader）
    TextRenderer(driver::IDisplay &lcd, IFontLoader &font_loader);
    ~TextRenderer() = default;

    // メンバ関数
    // パネル塗り→中央寄せ描画
    TextMetrics DrawLabel(int panel_x, int panel_y, int panel_w, int panel_h, const std::string &utf8, bool center = true);
    // (x,y) はベースライン基準（左下寄り）
    TextMetrics DrawText(int x, int y, const std::string &utf8);
    void SetColors(Color565 fg, Color565 bg);
    void SetFontSizePx(int px);
    void SetLineGapPx(int px);
    void SetWrapWidthPx(int px);  // 0 で折り返しなし

  private:
    // 型・エイリアス
    struct Glyph {
        int width{0};                // グリフビットマップの幅（ピクセル単位）
        int height{0};               // グリフビットマップの高さ（ピクセル単位）
        int left{0};                 // 描画開始位置の水平オフセット（ベースラインからの相対位置）
        int top{0};                  // 描画開始位置の垂直オフセット（ベースライン上からの高さ）
        int advance{0};              // 次の文字への水平移動量（ピクセル単位）
        int pitch{0};                // ビットマップの1行あたりのバイト数（パディング含む）
        std::vector<uint8_t> alpha;  // アンチエイリアス用グレースケールデータ（各ピクセルの不透明度 0-255）
    };
    using GlyphKey = uint64_t;

    // 定数
    static constexpr int kAlphaMax{255};
    static constexpr int kBlueShift{0};
    static constexpr int kBlueMask{0x1F};
    static constexpr int kGreenShift{5};
    static constexpr int kGreenMask{0x3F};
    static constexpr int kRedShift{11};
    static constexpr int kRedMask{0x1F};

    // UTF-8デコード用定数
    static constexpr unsigned char kUtf8AsciiMax{0b10000000};        // ASCII文字の最大値+1
    static constexpr unsigned char kUtf8TwoBytePrefix{0b110};        // 2バイト文字の先頭パターン (110xxxxx >> 5)
    static constexpr unsigned char kUtf8ThreeBytePrefix{0b1110};     // 3バイト文字の先頭パターン (1110xxxx >> 4)
    static constexpr unsigned char kUtf8FourBytePrefix{0b11110};     // 4バイト文字の先頭パターン (11110xxx >> 3)
    static constexpr unsigned char kUtf8TwoByteMask{0b11111};        // 2バイト文字の先頭バイトマスク
    static constexpr unsigned char kUtf8ThreeByteMask{0b1111};       // 3バイト文字の先頭バイトマスク
    static constexpr unsigned char kUtf8FourByteMask{0b111};         // 4バイト文字の先頭バイトマスク
    static constexpr unsigned char kUtf8ContinuationMask{0b111111};  // 継続バイトのマスク

    // UTF-8デコード用シフト量
    static constexpr int kUtf8TwoByteShift{5};            // 2バイト文字判定用のシフト量
    static constexpr int kUtf8ThreeByteShift{4};          // 3バイト文字判定用のシフト量
    static constexpr int kUtf8FourByteShift{3};           // 4バイト文字判定用のシフト量
    static constexpr int kUtf8ContinuationShift{6};       // 継続バイトのシフト量
    static constexpr int kUtf8TwoByteLeadingShift{6};     // 2バイト文字の先頭バイトのシフト量
    static constexpr int kUtf8ThreeByteLeadingShift{12};  // 3バイト文字の先頭バイトのシフト量
    static constexpr int kUtf8ThreeByte2ndByteShift{6};   // 3バイト文字の2番目のバイトのシフト量
    static constexpr int kUtf8FourByteLeadingShift{18};   // 4バイト文字の先頭バイトのシフト量
    static constexpr int kUtf8FourByte2ndByteShift{12};   // 4バイト文字の2番目のバイトのシフト量
    static constexpr int kUtf8FourByte3rdByteShift{6};    // 4バイト文字の3番目のバイトのシフト量

    // GlyphKey生成用定数
    static constexpr int kCodepointBits{21};             // コードポイント用のビット数
    static constexpr uint32_t kCodepointMask{0x1FFFFF};  // コードポイント用のマスク（21ビット分）

    // メンバ関数
    TextMetrics MeasureText(const std::string &utf8) const;
    static GlyphKey MakeKey(const int &size_px, const uint32_t &codepoint);
    uint16_t Blend565(const uint16_t &background, const uint16_t &foreground, const uint8_t &alpha);
    void blitGlyph(const int baseline_x, const int baseline_y, const Glyph &glyph);
    int ExtractColorComponent(const uint16_t &color, const int &shift, const int &mask);
    const Glyph *getGlyph(const uint32_t &codepoint);
    Glyph loadGlyph(const uint32_t &codepoint);
    static bool GetCodepoint(const std::string &utf8_str, size_t &index, uint32_t &codepoint);

    // メンバ変数
    Color565 background_color_{Color565::White()};
    Color565 foreground_color_{Color565::Black()};
    std::unordered_map<GlyphKey, Glyph> cache_;
    IFontLoader &font_loader_;  // フォントローディングの実装
    int font_size_px_{32};
    driver::IDisplay &lcd_;
    int line_gap_px_{4};
    int wrap_width_px_{0};
};

}  // namespace ui

#endif  // CYCOM_DISPLAY_TEXT_RENDERER_H_