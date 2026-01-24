#!/bin/bash

# エラーで即座に終了
set -e

# スクリプトがあるディレクトリの一つ上（プロジェクトルート）へ移動
cd "$(dirname "$0")/.."

# buildディレクトリが存在するか確認
if [ ! -d "build" ]; then
    echo "❌ build ディレクトリが見つかりません"
    echo "   先に ./scripts/build.sh を実行してください"
    exit 1
fi

cd build

# オプション解析
VERBOSE=false
SPECIFIC_TEST=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -v|--verbose)
            VERBOSE=true
            shift
            ;;
        -t|--test)
            SPECIFIC_TEST="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [options]"
            echo ""
            echo "Options:"
            echo "  -v, --verbose       詳細な出力を表示"
            echo "  -t, --test <name>   特定のテストのみを実行"
            echo "  -h, --help          このヘルプメッセージを表示"
            echo ""
            echo "Examples:"
            echo "  $0                                     # 全テストを実行"
            echo "  $0 -v                                  # 全テストを詳細モードで実行"
            echo "  $0 -t TextRendererTest                 # 特定のテストのみを実行"
            exit 0
            ;;
        *)
            echo "❌ 不明なオプション: $1"
            echo "   ヘルプを表示するには -h または --help を使用してください"
            exit 1
            ;;
    esac
done

echo "🧪 Running tests..."
echo "=================="

# テスト実行
if [ "$SPECIFIC_TEST" != "" ]; then
    echo "📝 Running specific test: $SPECIFIC_TEST"
    if [ "$VERBOSE" = true ]; then
        ctest -R "$SPECIFIC_TEST" --output-on-failure --verbose
    else
        ctest -R "$SPECIFIC_TEST" --output-on-failure
    fi
else
    if [ "$VERBOSE" = true ]; then
        ctest --output-on-failure --verbose
    else
        ctest --output-on-failure
    fi
fi

TEST_RESULT=$?

echo "=================="
if [ $TEST_RESULT -eq 0 ]; then
    echo "✅ All tests passed!"
else
    echo "❌ Some tests failed!"
    exit 1
fi
