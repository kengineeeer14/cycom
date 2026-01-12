# スレッド構成

## 概要

本プロジェクトでは、リアルタイム性とレスポンスを両立させるため、マルチスレッド設計を採用している。各スレッドは独立した責務を持ち、非同期で動作する。

## 複数スレッドを使う理由

- **センサ取得周期の安定化**: UART読み取りがブロッキングI/Oでも他の処理に影響しない
- **UIレスポンスの維持**: センサ取得やログ保存が遅くても画面表示を継続できる
- **並行処理の実現**: GPS受信、タッチ入力、データ記録、UI更新を同時実行
- **リソースの有効活用**: マルチコアCPUの活用

---

## スレッド一覧

| スレッド名 | 生成場所 | 実装場所 | 周期/動作 | 終了制御 |
|-----------|---------|---------|----------|----------|
| メインスレッド | [main.cc](../main.cc#L20) | main() | 1秒 | ✅ `std::atomic<bool>` |
| ロガースレッド | [main.cc](../main.cc#L40) | [logger.cc](../src/util/logger.cc#L52) | 設定値（デフォルト100ms） | ✅ `std::atomic<bool>` |
| UARTスレッド | [main.cc](../main.cc#L74) | main.cc（ラムダ） | 常時（ブロッキング） | ✅ `std::atomic<bool>` |
| タッチスレッド | [main.cc](../main.cc#L69) | [gt911.cc](../src/display/touch/gt911.cc#L42) | イベント駆動/ポーリング | ✅ `std::atomic<bool>` |

---

## 各スレッドの詳細

### 1. メインスレッド

**役割**: UI更新（速度表示の描画）

**処理内容**:
- GPSから速度データを取得（`gps.GetGnvtgSpeed()`）
- LCD画面へのテキスト描画
- 前回値との比較による差分更新

**周期**: 1秒（`UPDATE_INTERVAL = 1000ms`）

---

### 2. ロガースレッド

**役割**: GPSデータの定期的なCSVログ記録

**処理内容**:
- `logger.Start()` で起動
- コールバック関数内でGPSスナップショットを取得
- `log_on_` フラグがtrueの場合のみCSVへ書き込み

**周期**: 設定ファイル（`config/config.json`）の `log_interval_ms` で指定

**実装**:
- スレッド生成: [src/util/logger.cc](../src/util/logger.cc#L52)
- 終了制御: `std::atomic<bool> running_`
- 周期制御: `std::this_thread::sleep_until()` による精密な時刻管理

**終了処理**:
```cpp
void Logger::Stop() {
    bool was_running = running_.exchange(false, std::memory_order_acq_rel);
    if (was_running && th_.joinable()) th_.join();
}
```

**設計の利点**:
- コールバック関数による柔軟な処理注入
- `steady_clock` による周期ズレの累積防止
- デストラクタで自動的に安全終了

---

### 3. UARTスレッド

**役割**: GPS（L76K）からのNMEAデータ受信とパース

**処理内容**:
- UART経由で受信した生データをバッファリング
- 改行区切りでNMEA文を抽出
- `gps.ProcessNmeaLine()` でパース・状態更新

**動作**: `select()` でタイムアウト付き（100ms）の `read()` 実行

**実装**:
```cpp
std::thread uart_thread([&gps, fd]() {
    while (!util::g_shutdown_requested.load()) {
        // select()でタイムアウト付きread
        fd_set readfds;
        struct timeval tv = {0, 100000}; // 100ms
        FD_ZERO(&readfds);
        FD_SET(fd, &readfds);
        
        if (select(fd + 1, &readfds, nullptr, nullptr, &tv) > 0) {
            int n = ::read(fd, buf, sizeof(buf) - 1);
            // NMEA行の抽出とパース
        }
    }
});
```

**終了制御**:
- ✅ `util::g_shutdown_requested` フラグによる制御
- `select()` のタイムアウトで定期的に終了フラグをチェック
- エラー時の適切な処理

---

### 4. タッチスレッド

**役割**: タッチスクリーン（GT911）からの入力監視

**処理内容**:
- GPIOイベント（`gpiod::line_event`）またはポーリングでタッチ検出
- タッチ座標の読み取りと内部変数への保存
- `LastXY()` でメインスレッドから座標取得可能

**動作**:
- イベント駆動が優先（利用可能な場合）
- フォールバック: ポーリング（100ms周期）

**実装**:
- スレッド生成: [src/display/touch/gt911.cc](../src/display/touch/gt911.cc#L42)
- 終了制御: `std::atomic<bool> running_`
- 座標保持: `std::atomic<int> last_x_, last_y_`

**終了処理**:
```cpp
Touch::~Touch() {
    running_ = false;
    if (th_.joinable()) th_.join();
}
```

**設計の利点**:
- イベント駆動とポーリングの自動切り替え
- アトミック変数によるスレッドセーフな座標アクセス
- デストラクタによる自動クリーンアップ

---

## スレッド間の通信・同期

### データフロー

```
[UART Thread] ─→ gps (L76k) ←─ [Logger Thread]
                      ↓
                [Main Thread] (UI更新)

[Touch Thread] ─→ last_x_, last_y_ (atomic)
```

### 同期メカニズム

| スレッド間 | 共有データ | 同期方法 | 実装 |
|-----------|----------|---------|------|
| UART → Main/Logger | `gps` オブジェクト | 内部mutex | [gps_l76k.cc](../src/sensor/gps/gps_l76k.cc) |
| Touch → Main | タッチ座標 | `std::atomic<int>` | [gt911.h](../include/display/touch/gt911.h#L108) |
| Logger制御 | 実行フラグ | `std::atomic<bool>` | [logger.h](../include/util/logger.h#L50) |

**注意点**:
- `gps` オブジェクトは複数スレッドからアクセスされるため、内部で適切な排他制御が必要
- 現状では `gps.GetGnvtgSpeed()` や `gps.Snapshot()` の呼び出しが競合する可能性がある

