# Docker開発環境セットアップガイド

`docker/`ディレクトリには、サイクルコンピュータプロジェクトの完全なDocker開発環境が含まれています。

## 📦 含まれるもの

> **注意**: このDocker環境は開発専用です。本番環境（実機ハードウェア）では、アプリケーションを直接インストールして実行してください。

- **docker/Dockerfile**: Ubuntu 22.04ベースの開発環境
  - C++17ビルドツール（gcc, cmake, ninja）
  - clangd（IntelliSense用）
  - 必要なライブラリ（libfreetype, nlohmann-json等）
  - デバッグツール（gdb, valgrind等）
  
- **devcontainer.json**: VSCode Dev Container設定
  - C++開発に必要な拡張機能を自動インストール
  - clangd、CMake Tools、GitHub Copilot等
  
- **docker/docker-compose.yml**: コンテナオーケストレーション
  - ボリュームマウント
  - ハードウェアデバイスアクセス
  - ビルドキャッシュの永続化

## 🚀 クイックスタート

### 方法1: VSCode Dev Container（推奨）

1. VSCodeで`Remote - Containers`拡張機能をインストール
2. このプロジェクトをVSCodeで開く
3. コマンドパレット（Ctrl+Shift+P）で`Dev Containers: Reopen in Container`を選択
4. 初回はイメージビルドに数分かかります

→ コンテナ内でVSCodeが開き、すべての拡張機能と設定が適用されます。

### 方法2: Docker Compose

```bash
# イメージをビルド
cd docker
docker-compose build

# コンテナを起動
docker-compose up -d

# コンテナに入る
docker-compose exec devcontainer bash

# プロジェクトをビルド
cd /workspace
./scripts/build.sh

# 終了
docker-compose down
```

### 方法3: Dockerコマンド直接実行

```bash
# イメージをビルド
cd docker
docker build -t cycom-dev .

# コンテナを起動してシェルに入る
docker run -it --rm \
  -v "$(pwd):/workspace" \
  -v ~/.gitconfig:/home/developer/.gitconfig:ro \
  --privileged \
  cycom-dev bash

# ビルド
./scripts/build.sh
```

## 🔧 開発ワークフロー

### コンテナ内でのビルド

```bash
# CMakeでビルドシステム生成＋ビルド
./scripts/build.sh

# または手動で
mkdir -p build && cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..
make -j$(nproc)
```

### 実行

```bash
# ハードウェアモードで実行
./build/cycom

# モックモード（USE_HARDWARE=OFF）
cd build
cmake -DUSE_HARDWARE=OFF ..
make
./cycom
```

### コードフォーマット

```bash
# clang-formatで自動フォーマット
find src include -name "*.cc" -o -name "*.h" | xargs clang-format -i
```

### デバッグ

```bash
# gdbでデバッグ
gdb ./build/cycom

# valgrindでメモリリーク検出
valgrind --leak-check=full ./build/cycom
```

## 📁 ボリュームとマウント

- **プロジェクトディレクトリ**: `..` (docker-compose.yml基準) → `/workspace`
  - プロジェクトルート全体がコンテナ内の`/workspace`にマウントされます
- **Git設定**: `~/.gitconfig` → `/home/developer/.gitconfig` (読み取り専用)
- **SSH鍵**: `~/.ssh` → `/home/developer/.ssh` (読み取り専用)
- **ビルドキャッシュ**: Docker名前付きボリューム`cycom-build-cache` → `/workspace/build`
  - ビルド成果物を永続化し、コンテナ再作成時もキャッシュを保持

## ⚙️ カスタマイズ

### ユーザーID/GID変更

ホストとコンテナのUID/GIDを合わせることで権限問題を回避できます：

```bash
# docker-compose.ymlを編集
args:
  USER_UID: 1000  # あなたのUID
  USER_GID: 1000  # あなたのGID
```

### 追加パッケージのインストール

```bash
# コンテナ内で
sudo apt-get update
sudo apt-get install -y <package-name>

# または Dockerfile に追記して再ビルド
```

## 🧪 テスト実行

```bash
# テストビルドと実行（今後実装予定）
cd build
cmake -DBUILD_TESTING=ON ..
make
ctest --output-on-failure
```

## 📄 ライセンス

プロジェクト本体のライセンスに従います。
