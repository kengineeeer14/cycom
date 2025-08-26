cmake_minimum_required(VERSION 3.10)
project(cycom)

# C++17を使用
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# ヘッダファイルのディレクトリを指定
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    /usr/include
)

# ソースファイルのリスト
set(SOURCES
    main.cc
    src/hal/gpio/gpio_config.cc
    src/hal/uart/uart_config.cc
    src/sensor/gps/gps_l76k.cc
    src/util/logger.cc
)

# 実行ファイルを作成
add_executable(cycom ${SOURCES})

# libgpiod ライブラリをリンク
target_link_libraries(cycom gpiod)
