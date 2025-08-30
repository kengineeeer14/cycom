#ifndef CYCOM_DISPLAY_TEXT_RENDERER_H_
#define CYCOM_DISPLAY_TEXT_RENDERER_H_

#include <ft2build.h>
#include FT_FREETYPE_H

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

#include "display/st7796.h"

namespace ui {

struct Color565 {
    uint16_t value;  // RGB565
    static Color565 RGB(uint8_t r, uint8_t g, uint8_t b) {
        uint16_t v = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        return {v};
    }
    static Color565 Black() { return {0x0000}; }
    static Color565 White() { return {0xFFFF}; }
    static Color565 Gray()  { return {0x7BEF}; }
};

struct TextMetrics {
    int width_px;
    int height_px;
    int baseline_px;
};

class TextRenderer {
public:
    // font_path に .ttf / .otf を指定
    TextRenderer(st7796::Display& lcd, const std::string& font_path);
    ~TextRenderer();

    void SetFontSizePx(int px);
    void SetColors(Color565 fg, Color565 bg);
    void SetLineGapPx(int px);
    void SetWrapWidthPx(int px);  // 0 で折り返しなし

    // (x,y) はベースライン基準（左下寄り）
    TextMetrics DrawText(int x, int y, const std::string& utf8);
    TextMetrics MeasureText(const std::string& utf8) const;

    // パネル塗り→中央寄せ描画
    TextMetrics DrawLabel(int panel_x, int panel_y, int panel_w, int panel_h,
                          const std::string& utf8, bool center = true);

private:
    struct Glyph {
        int width = 0, height = 0;
        int left = 0, top = 0;     // bitmap_left/top
        int advance = 0;           // ピクセル
        int pitch = 0;             // row bytes
        std::vector<uint8_t> alpha; // 8bit alpha bitmap
    };
    using GlyphKey = uint64_t;
    static GlyphKey MakeKey(int size_px, uint32_t cp) {
        return (static_cast<uint64_t>(size_px) << 21) | (cp & 0x1FFFFF);
    }

    Glyph loadGlyph(uint32_t cp);
    const Glyph* getGlyph(uint32_t cp);

    static bool NextCodepoint(const std::string& s, size_t& i, uint32_t& cp);
    void blitGlyph(int dst_x, int dst_y, const Glyph& g);
    static inline uint16_t Blend565(uint16_t bg, uint16_t fg, uint8_t a);

private:
    st7796::Display& lcd_;
    FT_Library ft_ = nullptr;
    FT_Face face_  = nullptr;

    int font_size_px_ = 32;
    Color565 fg_ = Color565::Black();
    Color565 bg_ = Color565::White();
    int line_gap_px_ = 4;
    int wrap_width_px_ = 0;

    std::unordered_map<GlyphKey, Glyph> cache_;
};

} // namespace ui

#endif // CYCOM_DISPLAY_TEXT_RENDERER_H_