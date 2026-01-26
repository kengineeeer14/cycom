#ifndef CYCOM_DISPLAY_TEXT_RENDERER_H_
#define CYCOM_DISPLAY_TEXT_RENDERER_H_

#include <ft2build.h>
#include FT_FREETYPE_H

#include "driver/interface/i_display.h"

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace ui {

struct Color565 {
    uint16_t value;  // RGB565
    static Color565 RGB(uint8_t r, uint8_t g, uint8_t b) {
        uint16_t v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        return {v};
    }
    static Color565 Black() { return {0x0000}; }
    static Color565 White() { return {0xFFFF}; }
    static Color565 Gray() { return {0x7BEF}; }
};

class TextRenderer {
    // テスト用フレンドクラス
    friend class TextRendererTest;
    friend class TextRendererTest_Blend565_FullyOpaque_Test;
    friend class TextRendererTest_Blend565_FullyTransparent_Test;
    friend class TextRendererTest_Blend565_HalfTransparent_Test;
    friend class TextRendererTest_Blend565_SameColor_Test;
    friend class TextRendererTest_Blend565_MaxColorComponents_Test;
    friend class TextRendererTest_Blend565_PrimaryColors_Test;
    friend class TextRendererTest_ExtractColorComponentTest_Test;

  public:
    // 型・エイリアス
    struct TextMetrics {
        int width_px;
        int height_px;
        int baseline_px;
    };

    // コンストラクタ/デストラクタ
    // font_path に .ttf / .otf を指定
    TextRenderer(driver::IDisplay &lcd, const std::string &font_path);
    ~TextRenderer();

    // メンバ関数
    // パネル塗り→中央寄せ描画
    TextMetrics DrawLabel(int panel_x, int panel_y, int panel_w, int panel_h, const std::string &utf8, bool center = true);
    // (x,y) はベースライン基準（左下寄り）
    TextMetrics DrawText(int x, int y, const std::string &utf8);
    TextMetrics MeasureText(const std::string &utf8) const;
    void SetColors(Color565 fg, Color565 bg);
    void SetFontSizePx(int px);
    void SetLineGapPx(int px);
    void SetWrapWidthPx(int px);  // 0 で折り返しなし

  private:
    // 型・エイリアス
    struct Glyph {
        int width = 0, height = 0;
        int left = 0, top = 0;       // bitmap_left/top
        int advance = 0;             // ピクセル
        int pitch = 0;               // row bytes
        std::vector<uint8_t> alpha;  // 8bit alpha bitmap
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
    static constexpr unsigned char kUtf8AsciiMax{0x80};          // ASCII文字の最大値+1
    static constexpr unsigned char kUtf8TwoBytePrefix{0x6};      // 2バイト文字の先頭パターン (110xxxxx >> 5)
    static constexpr unsigned char kUtf8ThreeBytePrefix{0xE};    // 3バイト文字の先頭パターン (1110xxxx >> 4)
    static constexpr unsigned char kUtf8FourBytePrefix{0x1E};    // 4バイト文字の先頭パターン (11110xxx >> 3)
    static constexpr unsigned char kUtf8TwoByteMask{0x1F};       // 2バイト文字の先頭バイトマスク
    static constexpr unsigned char kUtf8ThreeByteMask{0x0F};     // 3バイト文字の先頭バイトマスク
    static constexpr unsigned char kUtf8FourByteMask{0x07};      // 4バイト文字の先頭バイトマスク
    static constexpr unsigned char kUtf8ContinuationMask{0x3F};  // 継続バイトのマスク

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

    // メンバ関数
    static GlyphKey MakeKey(int size_px, uint32_t cp) { return (static_cast<uint64_t>(size_px) << 21) | (cp & 0x1FFFFF); }

    /**
     * @brief RGB565形式の2色をアルファブレンディング（α合成）する
     *
     * @param[in] background 背景色（RGB565形式）
     * @param[in] foreground 前景色（RGB565形式）
     * @param[in] alpha アルファ値（0-255）
     * @return uint16_t ブレンド後の色（RGB565形式）
     */
    uint16_t Blend565(const uint16_t &background, const uint16_t &foreground, const uint8_t &alpha);
    void blitGlyph(int dst_x, int dst_y, const Glyph &g);
    int ExtractColorComponent(const uint16_t &color, const int &shift, const int &mask);
    const Glyph *getGlyph(uint32_t cp);
    Glyph loadGlyph(uint32_t cp);

    /**
     * @brief UTF-8 文字列から次のコードポイントを取得する
     *
     * @param[in] utf8_str UTF-8 文字列
     * @param[out] index 現在のインデックス（呼び出し後に次の位置に更新される）
     * @param[out] codepoint 取得したコードポイント
     * @return bool 成功した場合 true、失敗した場合 false
     */
    static bool NextCodepoint(const std::string &utf8_str, size_t &index, uint32_t &codepoint);

    // メンバ変数
    Color565 bg_ = Color565::White();
    std::unordered_map<GlyphKey, Glyph> cache_;
    FT_Face face_ = nullptr;
    Color565 fg_ = Color565::Black();
    int font_size_px_ = 32;
    FT_Library ft_ = nullptr;
    driver::IDisplay &lcd_;
    int line_gap_px_ = 4;
    int wrap_width_px_ = 0;
};

}  // namespace ui

#endif  // CYCOM_DISPLAY_TEXT_RENDERER_H_