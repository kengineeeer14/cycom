cmake_minimum_required(VERSION 3.10)
project(cycom)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 文字: 推奨は /usr/include を明示しないこと（自動で入る）
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# ★ FreeType を見つける
find_package(Freetype REQUIRED)

# src/*.cc を自動収集
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cc)

add_executable(cycom main.cc ${SRC_FILES})

# 既存ライブラリ + ★ FreeType をリンク
target_link_libraries(cycom
    gpiod
    pthread
    Freetype::Freetype   # ★ これを追加
)

# （任意）コンパイル時に FreeType のヘッダパスを明示したいなら：
# target_include_directories(cycom PRIVATE ${FREETYPE_INCLUDE_DIRS})