cmake_minimum_required(VERSION 3.10)
project(cycom)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# C++17を使用
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ヘッダファイルのディレクトリを指定
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    /usr/include
)

# ソースファイルの自動収集（src以下の .cc を再帰的に）
file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS ${PROJECT_SOURCE_DIR}/src/*.cc)

# 実行ファイルを作成（ルートの main.cc と合わせる）
add_executable(cycom main.cc ${SRC_FILES})

# libgpiod ライブラリをリンク
target_link_libraries(cycom gpiod pthread)
