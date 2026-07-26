#ifndef CYCOM_TESTS_MOCKS_DISPLAY_MOCK_FONT_LOADER_H_
#define CYCOM_TESTS_MOCKS_DISPLAY_MOCK_FONT_LOADER_H_

#include "presentation/display/interface/i_font_loader.h"

#include <gmock/gmock.h>

namespace presentation::display {

class MockFontLoader : public IFontLoader {
  public:
    MOCK_METHOD(int, LoadChar, (uint32_t codepoint, GlyphData &glyph_data), (override));
    MOCK_METHOD(void, SetPixelSize, (int size_px), (override));
    MOCK_METHOD(int, GetLineHeightPx, (), (const, override));
    MOCK_METHOD(int, GetAscentPx, (), (const, override));
    MOCK_METHOD(int, GetDescentPx, (), (const, override));
};

}  // namespace presentation::display

#endif  // CYCOM_TESTS_MOCKS_DISPLAY_MOCK_FONT_LOADER_H_
