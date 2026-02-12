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
    // FreeTypeのフォントメトリクスを取得し、ピクセル単位に変換
    // FreeTypeの値は26.6固定小数点形式（下位6ビットが小数部）のため、>>6で整数部を抽出
    int ascent{static_cast<int>(face_->size->metrics.ascender >> 6)};     // ベースラインから文字上端までの高さ
    int descent{-static_cast<int>(face_->size->metrics.descender >> 6)};  // ベースラインから文字下端までの深さ（正の値に反転）
    int line_h{static_cast<int>(face_->size->metrics.height >> 6)};       // 推奨される行の高さ
    if (line_h <= 0)
        line_h = ascent + descent + line_gap_px_;

    int wrap_w = wrap_width_px_;
    int cur_w = 0, max_w = 0, total_h = line_h;

    size_t i = 0;
    while (i < utf8.size()) {
        uint32_t cp;
        if (!GetCodepoint(utf8, i, cp))
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

void TextRenderer::SetColors(Color565 fg, Color565 bg) {
    foreground_color_ = fg;
    background_color_ = bg;
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

/**
 * @brief UTF-8文字列の描画に必要なメトリクス（幅・高さ・ベースライン位置）を計測する
 *
 * @param utf8_str UTF-8文字列
 * @return TextMetrics 描画に必要なメトリクス
 */
TextRenderer::TextMetrics TextRenderer::MeasureText(const std::string &utf8_str) const {
    int current_width_px{0};
    int max_width_px{0};
    // FreeTypeのフォントメトリクスを取得し、ピクセル単位に変換
    // FreeTypeの値は26.6固定小数点形式（下位6ビットが小数部）のため、>>6で整数部を抽出
    int ascent_px{static_cast<int>(face_->size->metrics.ascender >> 6)};  // ベースラインから文字上端までの高さ．大文字や上に伸びる文字（'A', 'h', 'b'など）の高さ．
    int line_height_px{static_cast<int>(face_->size->metrics.height >> 6)};  // 1行分の推奨される総高さです。次の行までの距離で、ascent + descent + 行間を含む．
    // フォントメトリクスが不正な場合（破損フォント、極小サイズ等）のフェイルセーフ
    // フォントサイズを基準に代替の行高さを計算して最低限の描画品質を保証
    if (line_height_px <= 0)
        line_height_px = font_size_px_ + line_gap_px_;  // TODO: エラー処理
    size_t i{0};
    while (i < utf8_str.size()) {
        uint32_t codepoint;
        if (!GetCodepoint(utf8_str, i, codepoint))
            // TODO: エラー処理
            break;
        if (codepoint == '\n') {
            // 改行文字の場合、現在の行幅を最大幅と比較し、行幅をリセットして次の行へ
            max_width_px = std::max(max_width_px, current_width_px);
            current_width_px = 0;
            continue;
        }
        current_width_px += static_cast<int>(font_size_px_ * 0.6);  // 各文字の幅をフォントサイズの60%で概算して加算
    }
    max_width_px = std::max(max_width_px, current_width_px);  // 最後の行の幅を最大幅と比較（最後の行の改行がない場合に対応）
    return TextMetrics{max_width_px, line_height_px, ascent_px};
}

/**
 * @brief サイズとコードポイントからキャッシュキーを生成する
 *
 * @param size_px フォントサイズ（ピクセル単位）
 * @param codepoint コードポイント
 * @return GlyphKey
 */
TextRenderer::GlyphKey TextRenderer::MakeKey(const int &size_px, const uint32_t &codepoint) {
    // 64ビットのキーを生成：上位43ビットにフォントサイズ、下位21ビットにコードポイントを格納
    // 例: size_px=32, codepoint=0x3042の場合
    //   → (32 << 21) | 0x3042 = 0x0000004000003042
    //
    // ビット配置:
    // [63....................21][20....................0]
    // [   フォントサイズ(px)   ][   コードポイント      ]
    return (static_cast<uint64_t>(size_px) << kCodepointBits) |  // フォントサイズを上位ビットに配置
           (codepoint & kCodepointMask);                         // コードポイントを下位21ビットに制限
}

/**
 * @brief RGB565形式の2色をアルファブレンディング（α合成）する
 *
 * @param[in] background 背景色（RGB565形式）
 * @param[in] foreground 前景色（RGB565形式）
 * @param[in] alpha アルファ値（0-255）
 * @return uint16_t ブレンド後の色（RGB565形式）
 */
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

/**
 * @brief 指定位置にグリフを描画する
 *
 * @param[in] baseline_x ベースライン上のX座標（ピクセル単位）
 * @param[in] baseline_y ベースラインのY座標（ピクセル単位）
 * @param[in] glyph 描画するグリフ
 */
void TextRenderer::blitGlyph(const int baseline_x, const int baseline_y, const Glyph &glyph) {
    if ((glyph.width <= 0) || (glyph.height <= 0))
        return;  // TODO: エラー処理
    const int screen_x{baseline_x + glyph.left};
    const int screen_y{baseline_y - glyph.top};

    // 1ラインずつα合成して送る
    std::vector<uint16_t> rgb565_line(glyph.width);
    for (int row{0}; row < glyph.height; ++row) {
        const uint8_t *alpha_row = glyph.alpha.data() + row * glyph.pitch;  // グリフのαデータの1行分(ベクトルの先頭アドレス)
        for (int col{0}; col < glyph.width; ++col) {
            uint8_t alpha{alpha_row[col]};
            rgb565_line[col] = Blend565(background_color_.value, foreground_color_.value, alpha);
        }
        lcd_.DrawRGB565Line(screen_x, screen_y + row, rgb565_line.data(), glyph.width);
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

/**
 * @brief UTF-8文字列の現在位置のコードポイントを取得し、インデックスを次の位置に更新する
 * @details UTF-8のバイト列→コードポイント変換の詳細は doc/utf8-explanation.md を参照
 *
 * @param[in] utf8_str UTF-8 文字列
 * @param[in,out] index 現在のインデックス（呼び出し後に次の位置に更新される）
 * @param[out] codepoint 取得したコードポイント
 * @return bool 成功した場合 true、失敗した場合 false
 *
 * @see doc/utf8-explanation.md
 */
bool TextRenderer::GetCodepoint(const std::string &utf8_str, size_t &index, uint32_t &codepoint) {
    bool is_valid{false};

    if (index >= utf8_str.size()) {
        is_valid = false;  // 文字列の終端に達した場合は次のコードポイントは取得できない
    } else {
        // 1バイト目を読み取り、文字の種類を判定
        const unsigned char first_byte{static_cast<unsigned char>(utf8_str[index++])};

        // ========== 1バイト文字（ASCII: 0xxxxxxx）==========
        if (first_byte < kUtf8AsciiMax) {  // 先頭ビットが0 → ASCII文字
            // 例: 'A' = 0x41 = 01000001
            codepoint = first_byte;
            is_valid = true;

            // ========== 2バイト文字（110xxxxx 10xxxxxx）==========
        } else if ((first_byte >> kUtf8TwoByteShift) == kUtf8TwoBytePrefix) {  // 先頭が110 → 2バイト文字
            // 例: 'α' (U+03B1) = 0xCE 0xB1 = 11001110 10110001
            if (index >= utf8_str.size()) {
                is_valid = false;  // 2バイト目がない
            } else {
                const unsigned char second_byte{static_cast<unsigned char>(utf8_str[index++])};

                // ビット抽出と結合:
                // 1バイト目: 110[xxxxx] → 下位5ビット (& 0b11111)
                // 2バイト目: 10[xxxxxx] → 下位6ビット (& 0b111111)
                // 結合: (1バイト目の5ビット << 6) | (2バイト目の6ビット)
                codepoint = ((first_byte & kUtf8TwoByteMask) << kUtf8TwoByteLeadingShift) | (second_byte & kUtf8ContinuationMask);
                is_valid = true;
            }

            // ========== 3バイト文字（1110xxxx 10xxxxxx 10xxxxxx）==========
        } else if ((first_byte >> kUtf8ThreeByteShift) == kUtf8ThreeBytePrefix) {  // 先頭が1110 → 3バイト文字
            // 例: 'あ' (U+3042) = 0xE3 0x81 0x82 = 11100011 10000001 10000010
            if (index + 1 > utf8_str.size()) {
                is_valid = false;  // 残りバイト数が不足
            } else {
                const unsigned char second_byte{static_cast<unsigned char>(utf8_str[index++])};
                const unsigned char third_byte{static_cast<unsigned char>(utf8_str[index++])};

                // ビット抽出と結合:
                // 1バイト目: 1110[xxxx] → 下位4ビット (& 0b1111) を12ビット左シフト
                // 2バイト目: 10[xxxxxx] → 下位6ビット (& 0b111111) を6ビット左シフト
                // 3バイト目: 10[xxxxxx] → 下位6ビット (& 0b111111)
                codepoint =
                    ((first_byte & kUtf8ThreeByteMask) << kUtf8ThreeByteLeadingShift) | ((second_byte & kUtf8ContinuationMask) << kUtf8ThreeByte2ndByteShift) | (third_byte & kUtf8ContinuationMask);
                is_valid = true;
            }

            // ========== 4バイト文字（11110xxx 10xxxxxx 10xxxxxx 10xxxxxx）==========
        } else if ((first_byte >> kUtf8FourByteShift) == kUtf8FourBytePrefix) {  // 先頭が11110 → 4バイト文字
            // 例: '🚴' (U+1F6B4) = 0xF0 0x9F 0x9A 0xB4 = 11110000 10011111 10011010 10110100
            if (index + 2 > utf8_str.size()) {
                is_valid = false;  // 残りバイト数が不足
            } else {
                const unsigned char second_byte{static_cast<unsigned char>(utf8_str[index++])};
                const unsigned char third_byte{static_cast<unsigned char>(utf8_str[index++])};
                const unsigned char fourth_byte{static_cast<unsigned char>(utf8_str[index++])};

                // ビット抽出と結合:
                // 1バイト目: 11110[xxx] → 下位3ビット (& 0b111) を18ビット左シフト
                // 2バイト目: 10[xxxxxx] → 下位6ビット (& 0b111111) を12ビット左シフト
                // 3バイト目: 10[xxxxxx] → 下位6ビット (& 0b111111) を6ビット左シフト
                // 4バイト目: 10[xxxxxx] → 下位6ビット (& 0b111111)
                codepoint = ((first_byte & kUtf8FourByteMask) << kUtf8FourByteLeadingShift) | ((second_byte & kUtf8ContinuationMask) << kUtf8FourByte2ndByteShift) |
                            ((third_byte & kUtf8ContinuationMask) << kUtf8FourByte3rdByteShift) | (fourth_byte & kUtf8ContinuationMask);
                is_valid = true;
            }
        } else {
            // 5バイト以上の文字や不正なパターン
            codepoint = '?';  // 不正な文字は'?'に置き換え
            is_valid = false;
        }
    }

    return is_valid;
}

}  // namespace ui