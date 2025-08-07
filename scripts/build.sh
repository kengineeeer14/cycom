#!/bin/bash

# スクリプトがあるディレクトリの一つ上（プロジェクトルート）へ移動
cd "$(dirname "$0")/.."

# buildディレクトリを作成
mkdir -p build
cd build

# CMake実行
cmake ..
make -j$(nproc)

# 実行ファイルができたか確認
if [ -f ./cycom ]; then
    echo "✅ Build successful: ./build/cycom"
else
    echo "❌ Build failed."
fi