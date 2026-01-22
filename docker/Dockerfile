# ビルド環境とランタイム環境を兼ねるイメージ
FROM ubuntu:22.04

# タイムゾーン設定（インタラクティブプロンプトを回避）
ENV DEBIAN_FRONTEND=noninteractive
ENV TZ=Asia/Tokyo

# 基本パッケージとビルドツールのインストール
RUN apt-get update && apt-get install -y \
    # ビルド必須ツール
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    # バージョン管理
    git \
    # 依存ライブラリ
    libfreetype6-dev \
    libgpiod-dev \
    nlohmann-json3-dev \
    # 注: 本番環境では実機に直接インストール \
    # デバッグ・開発ツール
    gdb \
    valgrind \
    clang \
    clang-format \
    clang-tidy \
    clangd \
    lldb \
    # ユーティリティ
    curl \
    wget \
    vim \
    nano \
    less \
    tree \
    htop \
    strace \
    # ドキュメント生成（オプション）
    doxygen \
    graphviz \
    # タイムゾーンデータ
    tzdata \
    # ロケール設定
    locales \
    && rm -rf /var/lib/apt/lists/*

# ロケール設定（日本語対応）
RUN locale-gen ja_JP.UTF-8
ENV LANG=ja_JP.UTF-8
ENV LANGUAGE=ja_JP:ja
ENV LC_ALL=ja_JP.UTF-8

# 非rootユーザーの作成（セキュリティとVSCode互換性のため）
ARG USERNAME=developer
ARG USER_UID=1000
ARG USER_GID=$USER_UID

RUN groupadd --gid $USER_GID $USERNAME \
    && useradd --uid $USER_UID --gid $USER_GID -m $USERNAME \
    && apt-get update \
    && apt-get install -y sudo \
    && echo $USERNAME ALL=\(root\) NOPASSWD:ALL > /etc/sudoers.d/$USERNAME \
    && chmod 0440 /etc/sudoers.d/$USERNAME \
    && rm -rf /var/lib/apt/lists/*

# Git設定（グローバル設定）
RUN git config --system pull.rebase true \
    && git config --system core.editor vim \
    && git config --system alias.ci commit \
    && git config --system alias.st status \
    && git config --system alias.br branch \
    && git config --system alias.co checkout

# git-prompt.sh のセットアップ（PS1でGitブランチ表示用）
RUN curl -fsSL https://raw.githubusercontent.com/git/git/master/contrib/completion/git-prompt.sh \
    -o /etc/bash_completion.d/git-prompt.sh \
    && chmod +x /etc/bash_completion.d/git-prompt.sh

# 作業ディレクトリの設定
WORKDIR /workspace

# ユーザーに所有権を付与
RUN chown -R $USERNAME:$USERNAME /workspace

# カスタムPS1設定ファイルをコピー
COPY ../.devcontainer/bashrc_custom.sh /etc/profile.d/bashrc_custom.sh
RUN chmod +r /etc/profile.d/bashrc_custom.sh

# 非rootユーザーに切り替え
USER $USERNAME

# .bashrcからカスタム設定を読み込む
RUN echo '' >> /home/$USERNAME/.bashrc \
    && echo '# Load custom prompt configuration' >> /home/$USERNAME/.bashrc \
    && echo 'if [ -f /etc/profile.d/bashrc_custom.sh ]; then' >> /home/$USERNAME/.bashrc \
    && echo '    source /etc/profile.d/bashrc_custom.sh' >> /home/$USERNAME/.bashrc \
    && echo 'fi' >> /home/$USERNAME/.bashrc

# デフォルトコマンド
CMD ["/bin/bash"]
