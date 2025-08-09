cmake_minimum_required(VERSION 3.10)
project(cycom)

# C++17を使用
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ヘッダファイルのディレクトリを指定
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# ソースファイルのリスト
set(SOURCES
    main.cc
    src/gpio/gpio_setting.cc
    src/sensors/uart/gps_l76k.cc
)

# 実行ファイルを作成
add_executable(cycom ${SOURCES})

# libgpiod ライブラリをリンク
target_link_libraries(cycom gpiod)
