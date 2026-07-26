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
    ${PROJECT_SOURCE_DIR}/src
)

# FreeTypeを見つける
find_package(Freetype REQUIRED)

# ソースファイルを収集
# HAL実装は本番/モックを切り替え
if(USE_HARDWARE)
    file(GLOB HAL_IMPL_FILES CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/src/hal/*.cc
    )
else()
    file(GLOB HAL_IMPL_FILES CONFIGURE_DEPENDS
        ${PROJECT_SOURCE_DIR}/tests/mocks/hal/*.cc
    )
endif()

file(GLOB DRIVER_IMPL_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/driver/*.cc
)
file(GLOB_RECURSE APPLICATION_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/application/*.cc
)
file(GLOB_RECURSE DOMAIN_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/domain/*.cc
)
file(GLOB_RECURSE PRESENTATION_FILES CONFIGURE_DEPENDS
    ${PROJECT_SOURCE_DIR}/src/presentation/*.cc
)
file(GLOB_RECURSE THIRD_PARTY_FILES CONFIGURE_DEPENDS 
    ${PROJECT_SOURCE_DIR}/src/third_party/*.cc
)

# 実行可能ファイル
add_executable(cycom 
    src/main.cc
    ${HAL_IMPL_FILES}
    ${DRIVER_IMPL_FILES}
    ${APPLICATION_FILES}
    ${DOMAIN_FILES}
    ${PRESENTATION_FILES}
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
