# サイクルコンピュータ アーキテクチャ

🚧 **TODO**: コードで正当性チェック（現在は雛形作成の状態）

## システム概要

本システムは、GPS センサーからデータを取得し、LCD ディスプレイに速度などの情報を表示する、Raspberry Pi ベースのサイクルコンピュータです。

## 全体構成

```mermaid
graph TB
    subgraph "main.cc (エントリポイント)"
        Main[Main Thread]
        UartThread[UART Thread]
    end

    subgraph "HAL (Hardware Abstraction Layer)"
        GPIO[GpioController<br/>GPIO制御]
        UART[UartConfigure<br/>UART設定]
        SPI[SpiController<br/>SPI通信]
        I2C[I2cController<br/>I2C通信]
        GPIOLine[GpioLine<br/>GPIOライン操作]
    end

    subgraph "Sensor"
        GPS[L76k<br/>GPSセンサー]
    end

    subgraph "Display"
        LCD[ST7796 Display<br/>LCD制御]
        Touch[GT911 Touch<br/>タッチコントローラ]
        TextRenderer[TextRenderer<br/>テキスト描画]
    end

    subgraph "Utility"
        Logger[Logger<br/>データロガー]
    end

    subgraph "External Hardware"
        HW_GPIO["/dev/gpiochip0"]
        HW_UART["/dev/ttyS0<br/>GPS Module L76K"]
        HW_SPI["/dev/spidev0.0<br/>ST7796 LCD"]
        HW_I2C["/dev/i2c-1<br/>GT911 Touch"]
    end

    subgraph "External Resources"
        Config["config/config.json"]
        Font["config/fonts/DejaVuSans.ttf"]
        BG["resource/background/*.jpg"]
        LogFile["log/*.csv"]
    end

    %% メインスレッドの依存関係
    Main --> GPIO
    Main --> UART
    Main --> GPS
    Main --> Logger
    Main --> LCD
    Main --> Touch
    Main --> TextRenderer

    %% UARTスレッドの依存関係
    UartThread --> GPS

    %% HALレイヤーの依存関係
    GPIO --> HW_GPIO
    UART --> HW_UART
    UART --> Config
    
    %% センサーの依存関係
    GPS --> HW_UART

    %% ディスプレイの依存関係
    LCD --> SPI
    LCD --> GPIOLine
    LCD --> BG
    SPI --> HW_SPI
    Touch --> I2C
    Touch --> GPIOLine
    I2C --> HW_I2C
    TextRenderer --> LCD
    TextRenderer --> Font
    GPIOLine --> HW_GPIO

    %% ロガーの依存関係
    Logger --> GPS
    Logger --> Config
    Logger --> LogFile

    %% スタイル定義
    classDef mainClass fill:#e1f5ff,stroke:#01579b,stroke-width:2px,color:#000
    classDef halClass fill:#fff9c4,stroke:#f57f17,stroke-width:2px,color:#000
    classDef sensorClass fill:#f3e5f5,stroke:#4a148c,stroke-width:2px,color:#000
    classDef displayClass fill:#e8f5e9,stroke:#1b5e20,stroke-width:2px,color:#000
    classDef utilClass fill:#fce4ec,stroke:#880e4f,stroke-width:2px,color:#000
    classDef hwClass fill:#eeeeee,stroke:#424242,stroke-width:2px,color:#000
    classDef resourceClass fill:#fff3e0,stroke:#e65100,stroke-width:2px,color:#000

    class Main,UartThread mainClass
    class GPIO,UART,SPI,I2C,GPIOLine halClass
    class GPS sensorClass
    class LCD,Touch,TextRenderer displayClass
    class Logger utilClass
    class HW_GPIO,HW_UART,HW_SPI,HW_I2C hwClass
    class Config,Font,BG,LogFile resourceClass
```

## コンポーネント詳細

### Main (main.cc)

システムのエントリポイント。以下の2つのスレッドで構成されます。

#### Main Thread
- 各コンポーネントの初期化
- LCD への背景画像表示
- 1秒周期での速度表示更新
- タッチイベント処理（将来的に）

#### UART Thread
- GPS モジュールから NMEA センテンスを読み取り
- `L76k::ProcessNmeaLine()` でデータをパース
- バッファリングして行単位で処理

### HAL (Hardware Abstraction Layer)

ハードウェアへの低レベルアクセスを抽象化するレイヤー。

- **GpioController**: GPIO の初期化と管理
- **UartConfigure**: UART ポートの設定とオープン
- **SpiController**: SPI 通信の制御
- **I2cController**: I2C 通信の制御
- **GpioLine**: 個々の GPIO ライン操作

### Sensor

#### L76k (GPS センサー)
- NMEA センテンス（GNRMC, GNVTG, GNGGA）のパース
- スレッドセーフなデータアクセス（mutex 使用）
- スナップショット機能による安全なデータ取得

### Display

#### ST7796 Display
- SPI 経由での LCD 制御
- 画面クリア、矩形描画、画像表示機能
- 320x480 解像度対応

#### GT911 Touch
- I2C 経由でのタッチコントローラ制御
- マルチタッチ対応（最大5点）
- 割り込みピン監視

#### TextRenderer
- FreeType ライブラリを使用した文字描画
- UTF-8 対応
- RGB565 カラーフォーマット
- 折り返し、中央寄せ対応

### Utility

#### Logger
- GPS データの CSV ファイル出力
- 設定可能なログ間隔
- コールバック機能による柔軟なログ処理

## データフロー

```mermaid
sequenceDiagram
    participant GPS_HW as GPS L76K<br/>(Hardware)
    participant UART as UART Thread
    participant GPS_SW as L76k<br/>(Software)
    participant Main as Main Thread
    participant TR as TextRenderer
    participant LCD_HW as ST7796 LCD<br/>(Hardware)
    participant Logger as Logger

    loop 常時
        GPS_HW->>UART: NMEA データ送信
        UART->>GPS_SW: ProcessNmeaLine()
        GPS_SW->>GPS_SW: データをパース・保存
    end

    loop 1秒周期
        Main->>GPS_SW: GetGnvtgSpeed()
        GPS_SW-->>Main: 速度データ
        Main->>TR: DrawLabel(速度文字列)
        TR->>LCD_HW: ピクセルデータ送信
    end

    loop ログ間隔
        Logger->>GPS_SW: Snapshot()
        GPS_SW-->>Logger: スナップショット
        Logger->>Logger: WriteCsv()
    end
```

## 処理フロー

### 初期化シーケンス

```mermaid
sequenceDiagram
    participant Main
    participant GPIO
    participant UART
    participant GPS
    participant LCD
    participant Touch
    participant Logger

    Main->>GPIO: SetupGpio()
    GPIO-->>Main: OK
    Main->>UART: SetupUart()
    UART-->>Main: fd (ファイルディスクリプタ)
    Main->>GPS: 構築
    Main->>Logger: Start(周期, コールバック)
    Logger-->>Main: スレッド起動
    Main->>LCD: DrawBackgroundImage()
    LCD-->>Main: OK
    Main->>Touch: 構築 (I2C初期化)
    Main->>Main: UARTスレッド起動
```

### メインループ

```mermaid
flowchart TD
    Start([メインループ開始]) --> Sleep[1秒待機]
    Sleep --> GetSpeed[GPS から速度取得]
    GetSpeed --> CheckChange{前回と<br/>異なる?}
    CheckChange -->|Yes| Draw[テキスト描画]
    CheckChange -->|No| Sleep
    Draw --> Sleep
```

## ファイル構成マップ

```mermaid
graph LR
    subgraph "include/"
        ICore[core/]
        IDisplay[display/]
        IHAL[hal/]
        ISensor[sensor/]
        IUtil[util/]
    end

    subgraph "src/"
        SCore[core/]
        SDisplay[display/]
        SHAL[hal/]
        SSensor[sensor/]
        SUtil[util/]
    end

    Main[main.cc]

    Main --> IDisplay
    Main --> IHAL
    Main --> ISensor
    Main --> IUtil

    IDisplay --> SDisplay
    IHAL --> SHAL
    ISensor --> SSensor
    IUtil --> SUtil
```

## 設定ファイル

### config/config.json
```json
{
  "uart": {
    "baudrate": 9600
  },
  "logger": {
    "log_interval_ms": 1000,
    "log_on": true
  }
}
```

## ハードウェア接続

| デバイス | インターフェース | デバイスファイル | 説明 |
|---------|----------------|-----------------|------|
| L76K GPS | UART | /dev/ttyS0 | GPSセンサー |
| ST7796 LCD | SPI | /dev/spidev0.0 | 3.5インチLCD |
| GT911 Touch | I2C | /dev/i2c-1 | タッチコントローラ |
| GPIO | GPIO | /dev/gpiochip0 | 汎用I/O制御 |

### GPIO ピン配置

| 機能 | BCM番号 | 説明 |
|-----|---------|------|
| LCD RST | 27 | LCDリセット |
| LCD DC | 22 | データ/コマンド選択 |
| LCD BL | 18 | バックライト制御 |
| Touch INT | 4 | タッチ割り込み |
| Touch RST | 1 | タッチリセット |
| UART RX | 15 | GPS受信 |
| UART TX | 14 | GPS送信 |

## スレッド構成

```mermaid
graph TD
    subgraph "プロセス"
        MainThread[Main Thread<br/>速度表示更新]
        UartThread[UART Thread<br/>GPS データ読取]
        LoggerThread[Logger Thread<br/>CSV書込]
    end

    MainThread -.共有データ.-> GPS_Data[(GPS Data<br/>mutex保護)]
    UartThread -.書込.-> GPS_Data
    LoggerThread -.読取.-> GPS_Data
```

## 依存ライブラリ

- **libgpiod**: GPIO 制御
- **nlohmann/json**: JSON パース
- **FreeType2**: フォント描画
- **stb_image**: 画像ロード
