#!/bin/bash

# エラーで即座に終了
set -e

# スクリプトがあるディレクトリの一つ上（プロジェクトルート）へ移動
cd "$(dirname "$0")/.."

# buildディレクトリを作成
mkdir -p build
cd build

# CMake実行
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

# 全ターゲット（メインアプリとテスト）をビルド
echo "🔨 Building all targets..."
if ! make -j$(nproc); then
    echo "❌ Build failed."
    exit 1
fi

# 実行ファイルとテストの確認
echo ""
echo "📦 Build Summary:"
echo "================="

if [ -f ./cycom ]; then
    echo "✅ Main app: ./build/cycom"
else
    echo "❌ Main app: ./build/cycom (not found)"
    exit 1
fi

# テスト実行ファイルの確認
if [ -d ./tests ]; then
    TEST_COUNT=0
    
    for test_exec in ./tests/*_test; do
        if [ -f "$test_exec" ] && [ -x "$test_exec" ]; then
            TEST_COUNT=$((TEST_COUNT + 1))
            TEST_NAME=$(basename "$test_exec")
            echo "✅ Test: ./build/tests/$TEST_NAME"
        fi
    done
    
    if [ $TEST_COUNT -eq 0 ]; then
        echo "⚠️  No test executables found"
    else
        echo ""
        echo "📊 Total: 1 app + $TEST_COUNT tests"
    fi
fi

echo "================="
echo "✅ Build completed successfully!"