cmake_minimum_required(VERSION 3.10)
project(cycom)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# デバッグビルドの設定
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

# デバッグシンボルと最適化の設定
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

# USE_HARDWAREフラグの設定（デフォルトはON、テスト時はOFF）
option(USE_HARDWARE "Enable hardware support" ON)
if(USE_HARDWARE)
    add_definitions(-DUSE_HARDWARE)
endif()

# インクルードパス
include_directories(
    ${PROJECT_SOURCE_DIR}/include
)

# FreeTypeを見つける
find_package(Freetype REQUIRED)

# ソースファイルを収集
# HAL実装は本番/モックを切り替え
if(USE_HARDWARE)
    file(GLOB_RECURSE HAL_IMPL_FILES CONFIGURE_DEPENDS 
        ${PROJECT_SOURCE_DIR}/src/hal/impl/*.cc
    )
else()
    file(GLOB_RECURSE HAL_IMPL_FILES CONFIGURE_DEPENDS 
        ${PROJECT_SOURCE_DIR}/tests/mocks/hal/impl/*.cc
    )
endif()

file(GLOB_RECURSE DRIVER_IMPL_FILES CONFIGURE_DEPENDS 
    ${PROJECT_SOURCE_DIR}/src/driver/impl/*.cc
)
file(GLOB_RECURSE UTIL_FILES CONFIGURE_DEPENDS 
    ${PROJECT_SOURCE_DIR}/src/util/*.cc
)
file(GLOB_RECURSE CORE_FILES CONFIGURE_DEPENDS 
    ${PROJECT_SOURCE_DIR}/src/core/*.cc
)
file(GLOB_RECURSE SENSOR_FILES CONFIGURE_DEPENDS 
    ${PROJECT_SOURCE_DIR}/src/sensor/*.cc
)
file(GLOB_RECURSE DISPLAY_FILES CONFIGURE_DEPENDS 
    ${PROJECT_SOURCE_DIR}/src/display/*.cc
)
file(GLOB_RECURSE THIRD_PARTY_FILES CONFIGURE_DEPENDS 
    ${PROJECT_SOURCE_DIR}/src/third_party/*.cc
)

# 実行可能ファイル
add_executable(cycom 
    main.cc 
    ${HAL_IMPL_FILES}
    ${DRIVER_IMPL_FILES}
    ${UTIL_FILES}
    ${CORE_FILES}
    ${SENSOR_FILES}
    ${DISPLAY_FILES}
    ${THIRD_PARTY_FILES}
)

# ライブラリをリンク
target_link_libraries(cycom
    gpiod
    pthread
    Freetype::Freetype
)

# テスト設定
enable_testing()
if(EXISTS "${PROJECT_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
endif()
