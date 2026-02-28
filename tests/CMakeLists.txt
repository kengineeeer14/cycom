cmake_minimum_required(VERSION 3.22)

# Google TestとGMockを見つける
find_package(GTest REQUIRED)
include(GoogleTest)

# テスト用の共通インクルードディレクトリ
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/tests
    ${GTEST_INCLUDE_DIRS}
    /usr/src/googletest/googlemock/include
)

# text_renderer_testのビルド
add_executable(text_renderer_test
    display/text_renderer_test.cc
    ${PROJECT_SOURCE_DIR}/src/display/color.cc
    ${PROJECT_SOURCE_DIR}/src/display/text_renderer.cc
    ${PROJECT_SOURCE_DIR}/src/display/impl/freetype_font_loader.cc
)

target_link_libraries(text_renderer_test
    ${GTEST_BOTH_LIBRARIES}
    gmock
    pthread
    Freetype::Freetype
)

# テストを登録
gtest_discover_tests(text_renderer_test)
