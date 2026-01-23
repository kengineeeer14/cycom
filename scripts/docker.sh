#!/bin/bash

# Docker開発環境のクイックスタートスクリプト
# このスクリプトはDocker環境のビルドと起動を簡単に行えます

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
DOCKER_DIR="$PROJECT_ROOT/docker"

if [ ! -d "$DOCKER_DIR" ]; then
    echo "Error: docker directory not found: $DOCKER_DIR" >&2
    exit 1
fi
cd "$DOCKER_DIR"

# カラー出力
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
NC='\033[0m' # No Color

# ヘルプメッセージ
show_help() {
    cat << EOF
Docker開発環境管理スクリプト

使い方:
    $0 [コマンド]

コマンド:
    build       Dockerイメージをビルド
    start       コンテナを起動（既存コンテナがあれば再利用）
    stop        コンテナを停止
    restart     コンテナを再起動
    shell       コンテナ内でbashシェルを起動
    clean       コンテナとボリュームを削除
    rebuild     完全にクリーンビルド（キャッシュなし）
    logs        コンテナのログを表示
    status      コンテナの状態を表示
    help        このヘルプを表示

例:
    $0 build        # イメージをビルド
    $0 start        # コンテナ起動
    $0 shell        # コンテナに入る
    $0 clean        # 全削除

EOF
}

# Dockerとdocker-composeの確認
check_docker() {
    if ! command -v docker &> /dev/null; then
        echo -e "${RED}エラー: Dockerがインストールされていません${NC}"
        exit 1
    fi
    
    if ! command -v docker-compose &> /dev/null && ! docker compose version &> /dev/null; then
        echo -e "${RED}エラー: docker-composeがインストールされていません${NC}"
        exit 1
    fi
}

# docker-compose or docker compose
DOCKER_COMPOSE_CMD="docker-compose"
if ! command -v docker-compose &> /dev/null; then
    DOCKER_COMPOSE_CMD="docker compose"
fi

# コマンド処理
case "${1:-help}" in
    build)
        echo -e "${GREEN}Dockerイメージをビルドしています...${NC}"
        check_docker
        $DOCKER_COMPOSE_CMD build
        echo -e "${GREEN}✅ ビルド完了${NC}"
        ;;
    
    start)
        echo -e "${GREEN}コンテナを起動しています...${NC}"
        check_docker
        $DOCKER_COMPOSE_CMD up -d
        echo -e "${GREEN}✅ コンテナ起動完了${NC}"
        echo -e "${YELLOW}シェルに入るには: $0 shell${NC}"
        ;;
    
    stop)
        echo -e "${YELLOW}コンテナを停止しています...${NC}"
        check_docker
        $DOCKER_COMPOSE_CMD stop
        echo -e "${GREEN}✅ コンテナ停止完了${NC}"
        ;;
    
    restart)
        echo -e "${YELLOW}コンテナを再起動しています...${NC}"
        check_docker
        $DOCKER_COMPOSE_CMD restart
        echo -e "${GREEN}✅ 再起動完了${NC}"
        ;;
    
    shell)
        check_docker
        if ! $DOCKER_COMPOSE_CMD ps | grep -q "cycom-dev.*Up"; then
            echo -e "${YELLOW}コンテナが起動していません。起動します...${NC}"
            $DOCKER_COMPOSE_CMD up -d
            sleep 2
        fi
        echo -e "${GREEN}コンテナに接続しています...${NC}"
        $DOCKER_COMPOSE_CMD exec devcontainer bash
        ;;
    
    clean)
        echo -e "${RED}コンテナとボリュームを削除しますか? [y/N]${NC}"
        read -r response
        if [[ "$response" =~ ^[Yy]$ ]]; then
            check_docker
            $DOCKER_COMPOSE_CMD down -v
            echo -e "${GREEN}✅ 削除完了${NC}"
        else
            echo -e "${YELLOW}キャンセルしました${NC}"
        fi
        ;;
    
    rebuild)
        echo -e "${YELLOW}完全にクリーンビルドします（キャッシュなし）${NC}"
        echo -e "${RED}続行しますか? [y/N]${NC}"
        read -r response
        if [[ "$response" =~ ^[Yy]$ ]]; then
            check_docker
            $DOCKER_COMPOSE_CMD down -v
            $DOCKER_COMPOSE_CMD build --no-cache
            echo -e "${GREEN}✅ リビルド完了${NC}"
        else
            echo -e "${YELLOW}キャンセルしました${NC}"
        fi
        ;;
    
    logs)
        check_docker
        $DOCKER_COMPOSE_CMD logs -f
        ;;
    
    status)
        check_docker
        echo -e "${GREEN}コンテナの状態:${NC}"
        $DOCKER_COMPOSE_CMD ps
        echo ""
        echo -e "${GREEN}ボリューム:${NC}"
        docker volume ls | grep cycom || echo "ボリュームなし"
        ;;
    
    help|--help|-h)
        show_help
        ;;
    
    *)
        echo -e "${RED}エラー: 不明なコマンド '$1'${NC}"
        echo ""
        show_help
        exit 1
        ;;
esac
