#ifndef CYCOM_DISPLAY_INTERFACE_I_FONT_LOADER_H_
#define CYCOM_DISPLAY_INTERFACE_I_FONT_LOADER_H_

#include <cstdint>
#include <vector>

namespace ui {

/**
 * @brief フォントローダーのインターフェース
 * @details FreeTypeライブラリへの依存を抽象化し、テスタビリティを向上させる
 */
class IFontLoader {
  public:
    /**
     * @brief グリフ情報を格納する構造体
     */
    struct GlyphData {
        int width{0};                // グリフビットマップの幅（ピクセル単位）
        int height{0};               // グリフビットマップの高さ（ピクセル単位）
        int left{0};                 // 描画開始位置の水平オフセット
        int top{0};                  // 描画開始位置の垂直オフセット
        int advance{0};              // 次の文字への水平移動量（ピクセル単位）
        int pitch{0};                // ビットマップの1行あたりのバイト数
        std::vector<uint8_t> alpha;  // アンチエイリアス用グレースケールデータ
    };

    virtual ~IFontLoader() = default;

    /**
     * @brief 指定されたコードポイントのグリフをロードする
     * @param codepoint ロードする文字のコードポイント
     * @param glyph_data ロードしたグリフデータを格納する変数
     * @return int 成功した場合は0、失敗した場合は非0
     */
    virtual int LoadChar(uint32_t codepoint, GlyphData &glyph_data) = 0;

    /**
     * @brief フォントサイズを設定する
     * @param size_px ピクセル単位のフォントサイズ
     */
    virtual void SetPixelSize(int size_px) = 0;

    /**
     * @brief フォントメトリクスの行の高さを取得する
     * @return int ピクセル単位の行の高さ
     */
    virtual int GetLineHeightPx() const = 0;

    /**
     * @brief フォントメトリクスのアセント（ベースラインから文字上端までの高さ）を取得する
     * @return int ピクセル単位のアセント
     */
    virtual int GetAscentPx() const = 0;

    /**
     * @brief フォントメトリクスのディセント（ベースラインから文字下端までの深さ）を取得する
     * @return int ピクセル単位のディセント（正の値）
     */
    virtual int GetDescentPx() const = 0;
};

}  // namespace ui

#endif  // CYCOM_DISPLAY_INTERFACE_I_FONT_LOADER_H_
