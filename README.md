# Cycle Computer (cycom)

サイクルコンピュータ向けC++プロジェクト - センサーデータ取り込み、ディスプレイ表示、ログ記録

## 🚀 クイックスタート

> **本番環境（実機ハードウェア）**: 実機では下記のローカル環境ビルド手順に従って、直接アプリケーションをインストール・実行してください。Docker環境は開発専用です。

### ローカル環境でのビルド（本番環境）

```bash
# 依存パッケージのインストール（Ubuntu/Debian）
sudo apt-get install -y build-essential cmake libfreetype6-dev libgpiod-dev

# ビルド
./scripts/build.sh

# 実行
./build/cycom
```

### Docker開発環境（推奨）

完全な開発環境がDockerコンテナで提供されています。

```bash
# クイックスタート
./scripts/docker.sh build    # イメージビルド
./scripts/docker.sh start    # コンテナ起動
./scripts/docker.sh shell    # コンテナに入る

# VSCode Dev Container
# VSCodeでプロジェクトを開き、
# "Reopen in Container" を選択
```

詳細は [Docker環境セットアップガイド](doc/docker-setup.md) を参照してください。

## 📁 プロジェクト構成

- `src/` - C++ソースコード
  - `core/` - コアロジック
  - `display/` - ディスプレイドライバ・レンダリング
  - `hal/` - ハードウェア抽象化レイヤ（GPIO, I2C, SPI, UART）
  - `sensor/` - センサー統合
- `include/` - 公開ヘッダー
- `tests/` - テストコード
- `config/` - 設定ファイル
- `scripts/` - ビルド・ユーティリティスクリプト
- `docker/` - Docker開発環境（Dockerfile、docker-compose.yml）
- `.devcontainer/` - VSCode Dev Container設定
- `doc/` - ドキュメント

## 🛠️ 開発

### ビルドモード

```bash
# ハードウェアモード（デフォルト）
cmake -DUSE_HARDWARE=ON ..

# モックモード（開発用）
cmake -DUSE_HARDWARE=OFF ..
```

### コードスタイル

Google C++ スタイルガイドに準拠。clang-formatで自動フォーマット：

```bash
find src include -name "*.cc" -o -name "*.h" | xargs clang-format -i
```

## 📖 ドキュメント

- [Docker環境セットアップ](doc/docker-setup.md)
- [スレッド設計](doc/thread.md)

## 📄 ライセンス

LICENSEファイルを参照してください。