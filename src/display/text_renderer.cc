#include "display/text_renderer.h"

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace ui {

// コンストラクタ/デストラクタ
TextRenderer::TextRenderer(driver::IDisplay &lcd, const std::string &font_path) : lcd_(lcd) {
    if (FT_Init_FreeType(&ft_) != 0)
        throw std::runtime_error("FT_Init_FreeType failed");
    if (FT_New_Face(ft_, font_path.c_str(), 0, &face_) != 0) {
        FT_Done_FreeType(ft_);
        ft_ = nullptr;
        throw std::runtime_error("FT_New_Face failed: " + font_path);
    }
    FT_Set_Pixel_Sizes(face_, 0, font_size_px_);
}

TextRenderer::~TextRenderer() {
    if (face_)
        FT_Done_Face(face_);
    if (ft_)
        FT_Done_FreeType(ft_);
}

// public メンバ関数
TextRenderer::TextMetrics TextRenderer::DrawLabel(int panel_x, int panel_y, int panel_w, int panel_h, const std::string &utf8, bool center) {
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

TextRenderer::TextMetrics TextRenderer::DrawText(int x, int y, const std::string &utf8) {
    int pen_x = x, pen_y = y;
    int ascent = (face_->size->metrics.ascender >> 6);
    int descent = -(face_->size->metrics.descender >> 6);
    int line_h = (face_->size->metrics.height >> 6);
    if (line_h <= 0)
        line_h = ascent + descent + line_gap_px_;

    int wrap_w = wrap_width_px_;
    int cur_w = 0, max_w = 0, total_h = line_h;

    size_t i = 0;
    while (i < utf8.size()) {
        uint32_t cp;
        if (!NextCodepoint(utf8, i, cp))
            break;

        if (cp == '\n') {
            max_w = std::max(max_w, cur_w);
            pen_x = x;
            pen_y += line_h + line_gap_px_;
            total_h += line_h + line_gap_px_;
            cur_w = 0;
            continue;
        }

        const Glyph *g = getGlyph(cp);
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

TextRenderer::TextMetrics TextRenderer::MeasureText(const std::string &utf8) const {
    int cur_w = 0, max_w = 0;
    int ascent = (face_->size->metrics.ascender >> 6);
    int line_h = (face_->size->metrics.height >> 6);
    if (line_h <= 0)
        line_h = font_size_px_ + line_gap_px_;

    size_t i = 0;
    while (i < utf8.size()) {
        uint32_t cp;
        if (!NextCodepoint(utf8, i, cp))
            break;
        if (cp == '\n') {
            max_w = std::max(max_w, cur_w);
            cur_w = 0;
            continue;
        }
        cur_w += static_cast<int>(font_size_px_ * 0.6);  // 概算
    }
    max_w = std::max(max_w, cur_w);
    return TextMetrics{max_w, line_h, ascent};
}

void TextRenderer::SetColors(Color565 fg, Color565 bg) {
    fg_ = fg;
    bg_ = bg;
}

void TextRenderer::SetFontSizePx(int px) {
    font_size_px_ = std::max(6, px);
    FT_Set_Pixel_Sizes(face_, 0, font_size_px_);
}

void TextRenderer::SetLineGapPx(int px) {
    line_gap_px_ = std::max(0, px);
}

void TextRenderer::SetWrapWidthPx(int px) {
    wrap_width_px_ = std::max(0, px);
}

// private メンバ関数
uint16_t TextRenderer::Blend565(const uint16_t &background, const uint16_t &foreground, const uint8_t &alpha) {
    // RGB565形式（16ビット）から、各色成分（R:5bit、G:6bit、B:5bit）を取り出す．
    const int background_red{ExtractColorComponent(background, kRedShift, kRedMask)};
    const int background_green{ExtractColorComponent(background, kGreenShift, kGreenMask)};
    const int background_blue{ExtractColorComponent(background, kBlueShift, kBlueMask)};
    const int foreground_red{ExtractColorComponent(foreground, kRedShift, kRedMask)};
    const int foreground_green{ExtractColorComponent(foreground, kGreenShift, kGreenMask)};
    const int foreground_blue{ExtractColorComponent(foreground, kBlueShift, kBlueMask)};

    // アルファ値 alpha (0-255) に基づいて、前景色と背景色を合成する．
    const int inverse_alpha{kAlphaMax - alpha};
    const int blended_red{(foreground_red * alpha + background_red * inverse_alpha) / kAlphaMax};
    const int blended_green{(foreground_green * alpha + background_green * inverse_alpha) / kAlphaMax};
    const int blended_blue{(foreground_blue * alpha + background_blue * inverse_alpha) / kAlphaMax};
    return static_cast<uint16_t>((blended_red << kRedShift) | (blended_green << kGreenShift) | blended_blue);
}

void TextRenderer::blitGlyph(int dst_x, int dst_y, const Glyph &g) {
    if (g.width <= 0 || g.height <= 0)
        return;
    int x0 = dst_x + g.left;
    int y0 = dst_y - g.top;

    // 1ラインずつα合成して送る
    std::vector<uint16_t> line(g.width);
    for (int y = 0; y < g.height; ++y) {
        const uint8_t *src = g.alpha.data() + y * g.pitch;
        for (int x = 0; x < g.width; ++x) {
            uint8_t a = src[x];
            line[x] = Blend565(bg_.value, fg_.value, a);
        }
        lcd_.DrawRGB565Line(x0, y0 + y, line.data(), g.width);
    }
}

/**
 * @brief RGB565形式の色から，指定された色成分（赤・緑・青）を抽出する
 *
 * @param[in] color RGB565形式の色
 * @param[in] shift 色成分のビットシフト量
 * @param[in] mask 色成分のマスク
 * @return int 抽出された色成分の値
 */
int TextRenderer::ExtractColorComponent(const uint16_t &color, const int &shift, const int &mask) {
    return (color >> shift) & mask;
}

const TextRenderer::Glyph *TextRenderer::getGlyph(uint32_t cp) {
    GlyphKey key = MakeKey(font_size_px_, cp);
    auto it = cache_.find(key);
    if (it != cache_.end())
        return &it->second;
    Glyph g = loadGlyph(cp);
    auto [pos, _] = cache_.emplace(key, std::move(g));
    return &pos->second;
}

TextRenderer::Glyph TextRenderer::loadGlyph(uint32_t cp) {
    Glyph g;
    if (FT_Load_Char(face_, cp, FT_LOAD_RENDER) != 0)
        return g;
    FT_GlyphSlot slot = face_->glyph;
    const FT_Bitmap &bmp = slot->bitmap;

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

bool TextRenderer::NextCodepoint(const std::string &utf8_str, size_t &index, uint32_t &codepoint) {
    bool is_valid{false};

    if (index >= utf8_str.size()) {
        is_valid = false;  // 文字列の終端に達した場合は次のコードポイントは取得できない
    } else {
        const unsigned char first_byte{static_cast<unsigned char>(utf8_str[index++])};
        if (first_byte < kUtf8AsciiMax) {  // 1バイト文字 (ASCII)
            codepoint = first_byte;
            is_valid = true;
        } else if ((first_byte >> kUtf8TwoByteShift) == kUtf8TwoBytePrefix) {  // 2バイト文字
            if (index >= utf8_str.size()) {
                is_valid = false;
            } else {
                const unsigned char second_byte{static_cast<unsigned char>(utf8_str[index++])};
                codepoint = ((first_byte & kUtf8TwoByteMask) << kUtf8TwoByteLeadingShift) | (second_byte & kUtf8ContinuationMask);
                is_valid = true;
            }
        } else if ((first_byte >> kUtf8ThreeByteShift) == kUtf8ThreeBytePrefix) {  // 3バイト文字
            if (index + 1 > utf8_str.size()) {
                is_valid = false;
            } else {
                const unsigned char second_byte{static_cast<unsigned char>(utf8_str[index++])};
                const unsigned char third_byte{static_cast<unsigned char>(utf8_str[index++])};
                codepoint =
                    ((first_byte & kUtf8ThreeByteMask) << kUtf8ThreeByteLeadingShift) | ((second_byte & kUtf8ContinuationMask) << kUtf8ThreeByte2ndByteShift) | (third_byte & kUtf8ContinuationMask);
                is_valid = true;
            }
        } else if ((first_byte >> kUtf8FourByteShift) == kUtf8FourBytePrefix) {  // 4バイト文字
            if (index + 2 > utf8_str.size()) {
                is_valid = false;
            } else {
                const unsigned char second_byte{static_cast<unsigned char>(utf8_str[index++])};
                const unsigned char third_byte{static_cast<unsigned char>(utf8_str[index++])};
                const unsigned char fourth_byte{static_cast<unsigned char>(utf8_str[index++])};
                codepoint = ((first_byte & kUtf8FourByteMask) << kUtf8FourByteLeadingShift) | ((second_byte & kUtf8ContinuationMask) << kUtf8FourByte2ndByteShift) |
                            ((third_byte & kUtf8ContinuationMask) << kUtf8FourByte3rdByteShift) | (fourth_byte & kUtf8ContinuationMask);
                is_valid = true;
            }
        } else {
            codepoint = '?';
            is_valid = true;
        }
    }

    return is_valid;
}

}  // namespace ui