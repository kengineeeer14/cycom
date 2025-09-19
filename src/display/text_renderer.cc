#include "display/text_renderer.h"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace ui {

inline uint16_t TextRenderer::Blend565(uint16_t bg, uint16_t fg, uint8_t a) {
    auto ex = [](uint16_t c, int sh, int m)->int { return (c >> sh) & m; };
    int br = ex(bg, 11, 0x1F), bgc = ex(bg, 5, 0x3F), bb = ex(bg, 0, 0x1F);
    int fr = ex(fg, 11, 0x1F), fgc = ex(fg, 5, 0x3F), fb = ex(fg, 0, 0x1F);
    int inv = 255 - a;
    int r = (fr * a + br * inv) / 255;
    int g = (fgc * a + bgc * inv) / 255;
    int b = (fb * a + bb * inv) / 255;
    return static_cast<uint16_t>((r << 11) | (g << 5) | b);
}

bool TextRenderer::NextCodepoint(const std::string& s, size_t& i, uint32_t& cp) {
    if (i >= s.size()) return false;
    unsigned char c0 = static_cast<unsigned char>(s[i++]);
    if (c0 < 0x80) { cp = c0; return true; }
    if ((c0 >> 5) == 0x6) { // 2B
        if (i >= s.size()) return false;
        unsigned char c1 = static_cast<unsigned char>(s[i++]);
        cp = ((c0 & 0x1F) << 6) | (c1 & 0x3F);
        return true;
    }
    if ((c0 >> 4) == 0xE) { // 3B
        if (i + 1 > s.size()) return false;
        unsigned char c1 = static_cast<unsigned char>(s[i++]);
        unsigned char c2 = static_cast<unsigned char>(s[i++]);
        cp = ((c0 & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
        return true;
    }
    if ((c0 >> 3) == 0x1E) { // 4B
        if (i + 2 > s.size()) return false;
        unsigned char c1 = static_cast<unsigned char>(s[i++]);
        unsigned char c2 = static_cast<unsigned char>(s[i++]);
        unsigned char c3 = static_cast<unsigned char>(s[i++]);
        cp = ((c0 & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
        return true;
    }
    cp = '?';
    return true;
}

TextRenderer::TextRenderer(st7796::Display& lcd, const std::string& font_path)
    : lcd_(lcd)
{
    if (FT_Init_FreeType(&ft_) != 0) throw std::runtime_error("FT_Init_FreeType failed");
    if (FT_New_Face(ft_, font_path.c_str(), 0, &face_) != 0) {
        FT_Done_FreeType(ft_); ft_ = nullptr;
        throw std::runtime_error("FT_New_Face failed: " + font_path);
    }
    FT_Set_Pixel_Sizes(face_, 0, font_size_px_);
}

TextRenderer::~TextRenderer() {
    if (face_) FT_Done_Face(face_);
    if (ft_) FT_Done_FreeType(ft_);
}

void TextRenderer::SetFontSizePx(const int &px) {
    font_size_px_ = std::max(min_font_size_, px);
    FT_Set_Pixel_Sizes(face_, 0, font_size_px_);
}

void TextRenderer::SetColors(const Color565 &fg, const Color565 &bg){
    fg_ = fg;
    bg_ = bg;
}

void TextRenderer::SetLineGapPx(const int &px) {
    line_gap_px_ = std::max(0, px);
}

void TextRenderer::SetWrapWidthPx(const int &px) {
    wrap_width_px_ = std::max(0, px);
}

TextRenderer::Glyph TextRenderer::loadGlyph(uint32_t cp) {
    Glyph g;
    if (FT_Load_Char(face_, cp, FT_LOAD_RENDER) != 0) return g;
    FT_GlyphSlot slot = face_->glyph;
    const FT_Bitmap& bmp = slot->bitmap;

    g.width = bmp.width;
    g.height = bmp.rows;
    g.left = slot->bitmap_left;
    g.top = slot->bitmap_top;
    g.advance = (slot->advance.x >> 6);
    g.pitch = bmp.pitch;

    if (g.width > 0 && g.height > 0) {
        g.alpha.resize(g.height * g.pitch);
        std::memcpy(g.alpha.data(), bmp.buffer, g.alpha.size());
    }
    return g;
}

const TextRenderer::Glyph* TextRenderer::getGlyph(uint32_t cp) {
    GlyphKey key = MakeKey(font_size_px_, cp);
    auto it = cache_.find(key);
    if (it != cache_.end()) return &it->second;
    Glyph g = loadGlyph(cp);
    auto [pos, _] = cache_.emplace(key, std::move(g));
    return &pos->second;
}

void TextRenderer::blitGlyph(int dst_x, int dst_y, const Glyph& g) {
    if (g.width <= 0 || g.height <= 0) return;
    int x0 = dst_x + g.left;
    int y0 = dst_y - g.top;

    // 1ラインずつα合成して送る
    std::vector<uint16_t> line(g.width);
    for (int y = 0; y < g.height; ++y) {
        const uint8_t* src = g.alpha.data() + y * g.pitch;
        for (int x = 0; x < g.width; ++x) {
            uint8_t a = src[x];
            line[x] = Blend565(bg_.value, fg_.value, a);
        }
        lcd_.DrawRGB565Line(x0, y0 + y, line.data(), g.width);
    }
}

TextMetrics TextRenderer::DrawText(int x, int y, const std::string& utf8) {
    int pen_x = x, pen_y = y;
    int ascent = (face_->size->metrics.ascender >> 6);
    int descent = -(face_->size->metrics.descender >> 6);
    int line_h = (face_->size->metrics.height >> 6);
    if (line_h <= 0) line_h = ascent + descent + line_gap_px_;

    int wrap_w = wrap_width_px_;
    int cur_w = 0, max_w = 0, total_h = line_h;

    size_t i = 0;
    while (i < utf8.size()) {
        uint32_t cp;
        if (!NextCodepoint(utf8, i, cp)) break;

        if (cp == '\n') {
            max_w = std::max(max_w, cur_w);
            pen_x = x;
            pen_y += line_h + line_gap_px_;
            total_h += line_h + line_gap_px_;
            cur_w = 0;
            continue;
        }

        const Glyph* g = getGlyph(cp);
        int adv = (g ? g->advance : font_size_px_ / 2);
        if (wrap_w > 0 && (pen_x - x + adv) > wrap_w) {
            max_w = std::max(max_w, cur_w);
            pen_x = x;
            pen_y += line_h + line_gap_px_;
            total_h += line_h + line_gap_px_;
            cur_w = 0;
        }

        if (g) {
            blitGlyph(pen_x, pen_y, *g);
            pen_x += g->advance;
            cur_w += g->advance;
        }
    }
    max_w = std::max(max_w, cur_w);

    return TextMetrics{max_w, total_h, ascent};
}

TextMetrics TextRenderer::MeasureText(const std::string& utf8) const {
    int cur_w = 0, max_w = 0;
    int ascent = (face_->size->metrics.ascender >> 6);
    int line_h = (face_->size->metrics.height >> 6);
    if (line_h <= 0) line_h = font_size_px_ + line_gap_px_;

    size_t i = 0;
    while (i < utf8.size()) {
        uint32_t cp;
        if (!NextCodepoint(utf8, i, cp)) break;
        if (cp == '\n') { max_w = std::max(max_w, cur_w); cur_w = 0; continue; }
        cur_w += static_cast<int>(font_size_px_ * 0.6); // 概算
    }
    max_w = std::max(max_w, cur_w);
    return TextMetrics{max_w, line_h, ascent};
}

TextMetrics TextRenderer::DrawLabel(int panel_x, int panel_y, int panel_w, int panel_h,
                                    const std::string& utf8, bool center) {
    int x1{panel_x + panel_w - 1};
    int y1{panel_y + panel_h - 1};
    if (center) {
        auto m = MeasureText(utf8);
        int x = panel_x + std::max(0, (panel_w - m.width_px) / 2);
        int y = panel_y + std::max(0, (panel_h + m.baseline_px) / 2);
        return DrawText(x, y, utf8);
    } else {
        int x = panel_x + 4;
        int y = panel_y + (font_size_px_ + 4);
        return DrawText(x, y, utf8);
    }
}

} // namespace ui