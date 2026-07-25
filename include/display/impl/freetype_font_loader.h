#ifndef CYCOM_DISPLAY_IMPL_FREETYPE_FONT_LOADER_H_
#define CYCOM_DISPLAY_IMPL_FREETYPE_FONT_LOADER_H_

#include "display/interface/i_font_loader.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include <string>

namespace ui {

/**
 * @brief FreeTypeライブラリを使用したフォントローダーの実装
 */
class FreeTypeFontLoader : public IFontLoader {
  public:
    /**
     * @brief コンストラクタ
     * @param font_path フォントファイルのパス
     * @throws std::runtime_error フォントの初期化に失敗した場合
     */
    explicit FreeTypeFontLoader(const std::string &font_path);

    /**
     * @brief デストラクタ
     */
    ~FreeTypeFontLoader() override;

    // IFontLoaderの実装
    int LoadChar(uint32_t codepoint, GlyphData &glyph_data) override;
    void SetPixelSize(int size_px) override;
    int GetLineHeightPx() const override;
    int GetAscentPx() const override;
    int GetDescentPx() const override;

  private:
    static constexpr int kFreeTypeFractionalBits{6};  // FreeTypeの26.6固定小数点形式における小数部のビット数

    FT_Library ft_{nullptr};
    FT_Face face_{nullptr};
};

}  // namespace ui

#endif  // CYCOM_DISPLAY_IMPL_FREETYPE_FONT_LOADER_H_
