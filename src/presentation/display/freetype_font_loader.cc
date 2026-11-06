#include "presentation/display/freetype_font_loader.h"

#include <cstring>
#include <stdexcept>

namespace presentation::display {

FreeTypeFontLoader::FreeTypeFontLoader(const std::string &font_path) {
    if (FT_Init_FreeType(&ft_) != 0) {
        throw std::runtime_error("FT_Init_FreeType failed");
    }

    if (FT_New_Face(ft_, font_path.c_str(), 0, &face_) != 0) {
        FT_Done_FreeType(ft_);
        ft_ = nullptr;
        throw std::runtime_error("FT_New_Face failed: " + font_path);
    }
}

FreeTypeFontLoader::~FreeTypeFontLoader() {
    if (face_) {
        FT_Done_Face(face_);
    }
    if (ft_) {
        FT_Done_FreeType(ft_);
    }
}

int FreeTypeFontLoader::LoadChar(uint32_t codepoint, GlyphData &glyph_data) {
    int result = FT_Load_Char(face_, codepoint, FT_LOAD_RENDER);

    if (result != 0) {
        // FT_Load_Charが失敗した場合
        return result;
    }

    // グリフデータを取得
    FT_GlyphSlot slot = face_->glyph;
    const FT_Bitmap &bmp = slot->bitmap;

    glyph_data.width = bmp.width;
    glyph_data.height = bmp.rows;
    glyph_data.left = slot->bitmap_left;
    glyph_data.top = slot->bitmap_top;
    glyph_data.advance = static_cast<int>(slot->advance.x >> kFreeTypeFractionalBits);
    glyph_data.pitch = bmp.pitch;

    if (glyph_data.width > 0 && glyph_data.height > 0) {
        // FreeTypeの内部バッファは次のFT_Load_Char呼び出し時に上書きされるため、
        // 画像データを独自のメモリ領域にコピーする
        glyph_data.alpha.resize(glyph_data.height * glyph_data.pitch);
        std::memcpy(glyph_data.alpha.data(), bmp.buffer, glyph_data.alpha.size());
    } else {
        // スペース文字などは画像データ不要
        glyph_data.alpha.clear();
    }

    return 0;
}

void FreeTypeFontLoader::SetPixelSize(int size_px) {
    FT_Set_Pixel_Sizes(face_, 0, size_px);
}

int FreeTypeFontLoader::GetLineHeightPx() const {
    return static_cast<int>(face_->size->metrics.height >> kFreeTypeFractionalBits);
}

int FreeTypeFontLoader::GetAscentPx() const {
    return static_cast<int>(face_->size->metrics.ascender >> kFreeTypeFractionalBits);
}

int FreeTypeFontLoader::GetDescentPx() const {
    // descender は負の値なので絶対値を返す
    return static_cast<int>(-face_->size->metrics.descender >> kFreeTypeFractionalBits);
}

}  // namespace presentation::display
