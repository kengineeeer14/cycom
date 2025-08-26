// =============================================================
// rpi_touch_lcd.cpp
// 目的: Raspberry Pi Zero 2 W 等で ST7796 搭載の SPI LCD と
//       GT911 タッチ(I2C)を最小構成で直接制御するサンプル。
// 概要:
//   - libgpiod: DC/RST/BL/INT のGPIO制御
//   - spidev  : ST7796 へのコマンド/データ送信
//   - i2c-dev : GT911 の16bitレジスタ読み書きと座標取得
//   - 6色バー描画、タッチ点の描画、未タッチ時のハートビート表示
// ビルド例:
//   g++ -std=c++17 rpi_touch_lcd.cpp -o rpi_touch_lcd -lgpiod -lpthread
// 注意:
//   - 配線(BCMピン)や /dev/spidev0.0 /dev/i2c-1 は環境に合わせて調整。
//   - 画面の向きやタッチの向きは MADCTL/座標反転で調整可能。
// =============================================================

// g++ -std=c++17 rpi_touch_lcd.cpp -o rpi_touch_lcd -lgpiod -lpthread
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <linux/spi/spidev.h>
#include <stdexcept>
#include <string>
#include <sys/ioctl.h>
#include <thread>
#include <unistd.h>
#include <vector>
#include <iostream>

// ===== GPIO (libgpiod) ユーティリティ =====
#include <gpiod.h>

// ---------------------------------------------
// GpioLine
//   libgpiod を使った 1 本の GPIO ライン用ラッパ。
//   - 出力として確保: request_output で 0/1 を駆動
//   - 入力として確保: 後から割り込み(立ち上がり)要求が可能
//   - waitEvent/readEvent でエッジイベントを取得
//   リソースは RAII で自動解放。
// ---------------------------------------------
class GpioLine {
public:
    // 指定チップ(例:"gpiochip0")とオフセットでラインを取得。
    // output=true なら出力ラインとして確保し初期値設定。
    // false の場合は後で requestRisingEdge() でイベントライン化。
    GpioLine(const char* chipname, unsigned int offset, bool output, int initial = 0)
    : chip_(nullptr), line_(nullptr), is_output_(output) {
        chip_ = gpiod_chip_open_by_name(chipname);       // "gpiochip0"
        if (!chip_) throw std::runtime_error("gpiod_chip_open_by_name failed");

        line_ = gpiod_chip_get_line(chip_, offset);
        if (!line_) throw std::runtime_error("gpiod_chip_get_line failed");

        if (output) {
            if (gpiod_line_request_output(line_, "rpi-touch", initial) < 0) {
                throw std::runtime_error("gpiod_line_request_output failed");
            }
        } else {
            // 入力ラインはここではリクエストしない。
            // 後で requestRisingEdge() でイベントラインとして取得する（EBUSY回避）。
        }
    }
    // 入力ラインを立ち上がりエッジでイベント待ちできるように設定する。
    // 既に他モードで確保されているとEBUSYになるため、一旦 release してから要求。
    void requestRisingEdge() {
        if (is_output_) {
            throw std::runtime_error("line is output");
        }
        // 念のため解放→イベント要求（先にinput取得しているとEBUSYになることがある）
        gpiod_line_release(line_);
        if (gpiod_line_request_rising_edge_events(line_, "rpi-touch") < 0) {
            throw std::runtime_error("gpiod_line_request_rising_edge_events failed");
        }
    }
    // 指定ミリ秒だけイベント(エッジ)を待つ。
    // >0: イベントあり, 0: タイムアウト, <0: エラー。
    int waitEvent(int timeout_ms) {
        timespec ts{timeout_ms/1000, (timeout_ms%1000)*1000000L};
        return gpiod_line_event_wait(line_, &ts);
    }
    // 直近のイベント(立ち上がりなど)の詳細を読み取る。
    void readEvent(gpiod_line_event* ev) {
        if (gpiod_line_event_read(line_, ev) < 0) {
            throw std::runtime_error("gpiod_line_event_read failed");
        }
    }
    // 出力ラインの値を設定する (0/1)。
    void set(int value) {
        if (!is_output_) throw std::runtime_error("line is not output");
        if (gpiod_line_set_value(line_, value) < 0) {
            throw std::runtime_error("gpiod_line_set_value failed");
        }
    }
    // ラインとチップを解放するRAIIデストラクタ。
    ~GpioLine() {
        if (line_) gpiod_line_release(line_);
        if (chip_) gpiod_chip_close(chip_);
    }
private:
    gpiod_chip* chip_;
    gpiod_line* line_;
    bool is_output_;
};

// ---------------------------------------------
// SPI
//   /dev/spidevX.Y を open し、モード/ビット幅/速度を設定して
//   writeBytes() で送信する薄いヘルパ。
//   ST7796 のコマンド/データ送信用に使用。
// ---------------------------------------------
class SPI {
public:
    // spidev デバイスを開き、SPI モード/ビット幅/クロックを設定。
    SPI(const char* dev, uint32_t speed_hz = 40000000, uint8_t mode = 0, uint8_t bits = 8)
    : fd_(-1) {
        fd_ = ::open(dev, O_RDWR);
        if (fd_ < 0) throw std::runtime_error("open spidev failed");

        if (ioctl(fd_, SPI_IOC_WR_MODE, &mode) < 0) throw std::runtime_error("SPI set mode failed");
        if (ioctl(fd_, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) throw std::runtime_error("SPI set bits failed");
        if (ioctl(fd_, SPI_IOC_WR_MAX_SPEED_HZ, &speed_hz) < 0) throw std::runtime_error("SPI set speed failed");
    }
    // 指定バッファを一括送信。送信バイト数が一致しない場合は例外。
    void writeBytes(const uint8_t* data, size_t len) {
        ssize_t w = ::write(fd_, data, len);
        if (w != (ssize_t)len) throw std::runtime_error("SPI write failed");
    }
    // ファイルディスクリプタをクローズ。
    ~SPI() { if (fd_ >= 0) ::close(fd_); }
private:
    int fd_;
};

// ---------------------------------------------
// I2C
//   /dev/i2c-1 を open し、I2C_RDWR ioctl で 16bit レジスタアクセス
//   (GT911 は 16bit レジスタ) を行うヘルパ。
//   - writeReg16: 連続書き込み
//   - readReg16 : Repeated Start で指定長読み出し
//   - read8 / write8: 1バイト版の糖衣
// ---------------------------------------------
class I2C {
public:
    // I2C デバイスを開き、互換のため I2C_SLAVE を設定(実際はI2C_RDWRを使用)。
    I2C(const char* dev, uint8_t addr) : fd_(-1), addr_(addr) {
        fd_ = ::open(dev, O_RDWR);
        if (fd_ < 0) throw std::runtime_error("open i2c failed");
        // I2C_RDWRで毎回addrを指定するので、I2C_SLAVEは必須ではないが、互換のため設定
        if (ioctl(fd_, I2C_SLAVE, addr_) < 0) throw std::runtime_error("I2C_SLAVE failed");
    }
    // 16bit レジスタアドレスに任意長データを書き込む。
    void writeReg16(uint16_t reg, const uint8_t* data, size_t len) {
        std::vector<uint8_t> buf(2 + len);
        buf[0] = static_cast<uint8_t>(reg >> 8);
        buf[1] = static_cast<uint8_t>(reg & 0xFF);
        std::memcpy(buf.data() + 2, data, len);

        i2c_msg msg{};
        msg.addr  = addr_;
        msg.flags = 0;
        msg.len   = static_cast<__u16>(buf.size());
        msg.buf   = buf.data();

        i2c_rdwr_ioctl_data ioctl_data{};
        ioctl_data.msgs  = &msg;
        ioctl_data.nmsgs = 1;

        if (ioctl(fd_, I2C_RDWR, &ioctl_data) < 0) throw std::runtime_error("I2C_RDWR write failed");
    }
    // 16bit レジスタアドレスを先に書いてから、Repeated Start で len バイト読む。
    void readReg16(uint16_t reg, uint8_t* out, size_t len) {
        uint8_t addrbuf[2] = { static_cast<uint8_t>(reg >> 8), static_cast<uint8_t>(reg & 0xFF) };

        i2c_msg msgs[2]{};
        // write register address
        msgs[0].addr  = addr_;
        msgs[0].flags = 0;
        msgs[0].len   = 2;
        msgs[0].buf   = addrbuf;
        // read data
        msgs[1].addr  = addr_;
        msgs[1].flags = I2C_M_RD;
        msgs[1].len   = static_cast<__u16>(len);
        msgs[1].buf   = out;

        i2c_rdwr_ioctl_data ioctl_data{};
        ioctl_data.msgs  = msgs;
        ioctl_data.nmsgs = 2;

        if (ioctl(fd_, I2C_RDWR, &ioctl_data) < 0) throw std::runtime_error("I2C_RDWR read failed");
    }
    // 1バイト読み出しのショートカット。
    uint8_t read8(uint16_t reg) {
        uint8_t v{};
        readReg16(reg, &v, 1);
        return v;
    }
    // 1バイト書き込みのショートカット。
    void write8(uint16_t reg, uint8_t v) { writeReg16(reg, &v, 1); }
    ~I2C() { if (fd_ >= 0) ::close(fd_); }
private:
    int fd_;
    uint8_t addr_;
};

// =============================================================
// ST7796 LCD ドライバ領域
//   - 画面サイズや制御用 GPIO(DC/RST/BL) を定義
//   - Display クラスで初期化・矩形塗りつぶし・全画面転送を提供
// =============================================================
namespace st7796 {

static constexpr int kWidth  = 320;
static constexpr int kHeight = 480;

// ピン（BCM）
static constexpr unsigned kRST = 27;
static constexpr unsigned kDC  = 22;
static constexpr unsigned kBL  = 18;

// Display
//   ST7796 向けの最小機能ドライバ。
//   コンストラクタでリセット/初期化し BL(バックライト) を ON。
//   主な公開API:
//     - clear(rgb565): 全画面を指定色で消去
//     - drawFilledRect(x0,y0,x1,y1,color): 矩形塗りつぶし
//     - blitRGB565(buf,len): 生 RGB565 バッファを全画面描画
class Display {
public:
    // DC/RST/BL のGPIOと SPI を初期化し、LCD をリセット→初期化→BL点灯。
    Display()
    : dc_("gpiochip0", kDC, true, 1),
      rst_("gpiochip0", kRST, true, 1),
      bl_("gpiochip0", kBL, true, 1),
      spi_("/dev/spidev0.0", 40000000, 0, 8)  // 必要なら 12000000 などに下げてテスト
    {
        reset();
        init();
        // BLはとりあえず常時ON
        bl_.set(1);
    }

    // 画面全体を指定色でクリアする。行単位に同じ色データを送る。
    void clear(uint16_t rgb565 = 0xFFFF) {
        setAddressWindow(0, 0, kWidth-1, kHeight-1);
        dataMode(true);
        std::vector<uint8_t> line(kWidth * 2);
        for (int y = 0; y < kHeight; ++y) {
            for (int x = 0; x < kWidth; ++x) {
                line[2*x + 0] = (rgb565 >> 8) & 0xFF;
                line[2*x + 1] = (rgb565     ) & 0xFF;
            }
            sendChunked(line.data(), line.size());
        }
    }

    // 指定矩形領域をRGB565で塗りつぶす。範囲を画面内にクリップしてから送信。
    void drawFilledRect(int x0, int y0, int x1, int y1, uint16_t rgb565) {
        if (x0 > x1) std::swap(x0, x1);
        if (y0 > y1) std::swap(y0, y1);
        x0 = std::max(0, x0); y0 = std::max(0, y0);
        x1 = std::min(kWidth-1,  x1); y1 = std::min(kHeight-1, y1);

        // If the rectangle is completely outside the screen after clipping, do nothing.
        if (x0 > x1 || y0 > y1) {
            return;
        }

        setAddressWindow(x0, y0, x1, y1);
        dataMode(true);

        const int w = (x1 - x0 + 1);
        std::vector<uint8_t> line(w * 2);
        for (int i = 0; i < w; ++i) {
            line[2*i+0] = (rgb565 >> 8) & 0xFF;
            line[2*i+1] = (rgb565     ) & 0xFF;
        }
        for (int y = y0; y <= y1; ++y) {
            sendChunked(line.data(), line.size());
        }
    }

    // (幅*高さ*2) バイトの生RGB565バッファを画面全体に一括で転送する。
    void blitRGB565(const uint8_t* buf, size_t len) {
        const size_t expected = size_t(kWidth)*kHeight*2;
        if (len != expected) throw std::runtime_error("blitRGB565: size mismatch");
        setAddressWindow(0, 0, kWidth-1, kHeight-1);
        dataMode(true);
        sendChunked(buf, len);
    }

private:
    // DC(GPIO)でコマンド/データを切り替える (false=CMD, true=DATA)。
    void dataMode(bool data) { dc_.set(data ? 1 : 0); }

    // 単一コマンド送信。
    void cmd(uint8_t c) {
        dataMode(false);
        spi_.writeBytes(&c, 1);
    }
    // 単一データ送信。
    void dat(uint8_t d) {
        dataMode(true);
        spi_.writeBytes(&d, 1);
    }

    // ハードウェアリセットシーケンス。
    void reset() {
        rst_.set(1); usleep(10000);
        rst_.set(0); usleep(10000);
        rst_.set(1); usleep(10000);
    }

    // 書き込み対象のアドレスウィンドウ(CASET/RASET)を設定し、RAMWRを発行。
    void setAddressWindow(int xs, int ys, int xe, int ye) {
        // CASET (0x2A): X
        cmd(0x2A);
        dat((xs >> 8) & 0xFF); dat(xs & 0xFF);
        dat((xe >> 8) & 0xFF); dat(xe & 0xFF);
        // RASET (0x2B): Y
        cmd(0x2B);
        dat((ys >> 8) & 0xFF); dat(ys & 0xFF);
        dat((ye >> 8) & 0xFF); dat(ye & 0xFF);
        // RAMWR
        cmd(0x2C);
    }

    // 大きな転送をチャンクに分けてSPI送信する安全策(4096バイト単位)。
    void sendChunked(const uint8_t* data, size_t len) {
        const size_t CHUNK = 4096;
        size_t off = 0;
        while (off < len) {
            size_t n = std::min(CHUNK, len - off);
            spi_.writeBytes(data + off, n);
            off += n;
        }
    }

    // ST7796 初期化シーケンス。ピクセルフォーマットやガンマ/電源設定、
    // MADCTL(画面向き)などを設定し、最終的に display on。
    void init() {
        // Sleep Out
        cmd(0x11); usleep(120000);

        // MADCTL: 縦（0x08）。必要なら 0x68 / 0xA8 なども試す
        cmd(0x36); dat(0x08);

        // Pixel format: 16bpp
        cmd(0x3A); dat(0x55); // 0x55=16bit/px

        // 以降、Python版に寄せた初期値
        cmd(0xF0); dat(0xC3);
        cmd(0xF0); dat(0x96);
        cmd(0xB4); dat(0x01);
        cmd(0xB7); dat(0xC6);
        cmd(0xC0); dat(0x80); dat(0x45);
        cmd(0xC1); dat(0x13);
        cmd(0xC2); dat(0xA7);
        cmd(0xC5); dat(0x0A);

        cmd(0xE8);
        for (uint8_t v : std::array<uint8_t,8>{0x40,0x8A,0x00,0x00,0x29,0x19,0xA5,0x33}) dat(v);

        cmd(0xE0);
        for (uint8_t v : std::array<uint8_t,14>{0xD0,0x08,0x0F,0x06,0x06,0x33,0x30,0x33,0x47,0x17,0x13,0x13,0x2B,0x31}) dat(v);

        cmd(0xE1);
        for (uint8_t v : std::array<uint8_t,14>{0xD0,0x0A,0x11,0x0B,0x09,0x07,0x2F,0x33,0x47,0x38,0x15,0x16,0x2C,0x32}) dat(v);

        cmd(0xF0); dat(0x3C);
        cmd(0xF0); dat(0x69);

        cmd(0x21); // inversion on
        cmd(0x11); usleep(100000);
        cmd(0x29); // display on
    }

private:
    GpioLine dc_, rst_, bl_;
    SPI      spi_;
};

} // namespace st7796

// =============================================================
// GT911 タッチドライバ領域
//   - 主に16bitレジスタマップを用いた設定/座標取得
//   - Touch クラスが、割り込み(可能なら)またはポーリングで座標を更新
// =============================================================
namespace gt911 {

// レジスタ
static constexpr uint16_t COMMAND_REG            = 0x8040;
static constexpr uint16_t GT_CFG_VERSION_REG     = 0x8047;
static constexpr uint16_t X_OUTPUT_MAX_LOW_REG   = 0x8048;
static constexpr uint16_t X_OUTPUT_MAX_HIGH_REG  = 0x8049;
static constexpr uint16_t Y_OUTPUT_MAX_LOW_REG   = 0x804A;
static constexpr uint16_t Y_OUTPUT_MAX_HIGH_REG  = 0x804B;
static constexpr uint16_t TOUCH_NUMBER_REG       = 0x804C;
static constexpr uint16_t COORDINATE_INFO_REG    = 0x814E;
static constexpr uint16_t POINT_1_X_LOW_REG      = 0x8150;
static constexpr uint16_t POINT_1_X_HIGH_REG     = 0x8151;
static constexpr uint16_t POINT_1_Y_LOW_REG      = 0x8152;
static constexpr uint16_t POINT_1_Y_HIGH_REG     = 0x8153;
static constexpr uint16_t GT_CFG_START_REG       = 0x8047;
static constexpr uint16_t GT_CHECKSUM_REG        = 0x80FF;
static constexpr uint16_t GT_FLASH_SAVE_REG      = 0x8100;

// 画面サイズ（Pythonと合わせる）
static constexpr int COORDINATE_X_MAX = 320;
static constexpr int COORDINATE_Y_MAX = 480;

// ピン（BCM）
static constexpr unsigned kGT911_INT_PIN = 4;
// ※要確認：環境によってRSTはBCM17等に変更してください（BCM1は不適な基板あり）
static constexpr unsigned kGT911_RST_PIN = 1;

// Touch
//   GT911 の初期化と座標読み取りを担当するクラス。
//   コンストラクタでリセット/解像度設定/動作開始し、
//   割り込みが使えればイベント待ち、難しければポーリングに自動フォールバック。
//   最新座標は lastXY() で参照(無効時は負値)。
class Touch {
public:
    // I2C/GPIO を初期化し、GT911 をリセット→CFG→ランに移行。
    // INT ピンに対して立ち上がりイベントを要求し、失敗時はポーリングに切替。
    // 背景スレッドで irqLoop() を回す。
    Touch(uint8_t i2c_addr = 0x5D)
    : i2c_("/dev/i2c-1", i2c_addr),
      rst_("gpiochip0", kGT911_RST_PIN, true, 1),
      int_in_("gpiochip0", kGT911_INT_PIN, false),
      use_events_(false),
      running_(true), last_x_(-1), last_y_(-1)
    {
        // ハードリセット
        reset();

        // 動作モード一旦停止→CFG→動作モード
        i2c_.write8(COMMAND_REG, 0x02); // reset/stop
        usleep(10000);

        // 必要ならここで設定（X/Y最大値）
        configureResolution(COORDINATE_X_MAX, COORDINATE_Y_MAX);

        // 動作へ
        i2c_.write8(COMMAND_REG, 0x00);
        usleep(50000);

        // 割り込みライン（立ち上がり）要求。失敗したらポーリングへフォールバック
        try {
            int_in_.requestRisingEdge();
            use_events_ = true;
        } catch (...) {
            use_events_ = false;
        }

        // スレッド開始（イベント or ポーリング）
        th_ = std::thread([this]{ this->irqLoop(); });
    }

    // スレッド停止と join を行うデストラクタ(RAII)。
    ~Touch() {
        running_ = false;
        if (th_.joinable()) th_.join();
    }

    // 直近に読み取ったタッチ座標を返す(無効時は{-1,-1})。
    std::pair<int,int> lastXY() const {
        return { last_x_.load(), last_y_.load() };
    }

private:
    // GT911 のハードリセット(簡易シーケンス)。
    void reset() {
        rst_.set(0); usleep(10000);
        rst_.set(1); usleep(50000);
    }

    // タッチ報告のX/Y最大値をレジスタに設定。LCDの実解像度と合わせる。
    void configureResolution(int x_max, int y_max) {
        i2c_.write8(X_OUTPUT_MAX_LOW_REG,  x_max & 0xFF);
        i2c_.write8(X_OUTPUT_MAX_HIGH_REG, (x_max >> 8) & 0x0F);
        i2c_.write8(Y_OUTPUT_MAX_LOW_REG,  y_max & 0xFF);
        i2c_.write8(Y_OUTPUT_MAX_HIGH_REG, (y_max >> 8) & 0x0F);
    }

    // バックグラウンドでイベント待ち(またはポーリング)し、
    // タッチがあれば handleTouch() で座標を更新するメインループ。
    void irqLoop() {
        while (running_) {
            if (use_events_) {
                int r = int_in_.waitEvent(200); // 200ms timeout
                if (r > 0) {
                    gpiod_line_event ev{};
                    try {
                        int_in_.readEvent(&ev);
                        handleTouch();
                    } catch (...) {
                        // ignore and continue
                    }
                }
            } else {
                // Polling fallback: ~50Hz
                handleTouch();
                usleep(20000);
            }
        }
    }

    // COORDINATE_INFO_REG から接触数/フラグを読み、1点目の座標を取得。
    // 画面の左右反転を適用し、last_x_/last_y_ を更新してステータスをクリア。
    void handleTouch() {
        uint8_t info = 0;
        try {
            info = i2c_.read8(COORDINATE_INFO_REG);
        } catch (...) { return; }

        uint8_t touch_num = info & 0x0F;
        if (touch_num == 0) {
            last_x_.store(-1);
            last_y_.store(-1);
            clearStatus();
            return;
        }

        uint8_t x_lo=0,x_hi=0,y_lo=0,y_hi=0;
        try {
            x_lo = i2c_.read8(POINT_1_X_LOW_REG);
            x_hi = i2c_.read8(POINT_1_X_HIGH_REG);
            y_lo = i2c_.read8(POINT_1_Y_LOW_REG);
            y_hi = i2c_.read8(POINT_1_Y_HIGH_REG);
        } catch (...) { return; }

        int x = ((int)x_hi << 8) | x_lo;
        int y = ((int)y_hi << 8) | y_lo;

        // Apply horizontal mirror (off-by-one corrected) and clamp to panel bounds
        x = COORDINATE_X_MAX - 1 - x;
        if (x < 0) x = 0; else if (x >= COORDINATE_X_MAX) x = COORDINATE_X_MAX - 1;
        if (y < 0) y = 0; else if (y >= COORDINATE_Y_MAX) y = COORDINATE_Y_MAX - 1;

        last_x_.store(x);
        last_y_.store(y);

        clearStatus();
    }

    // タッチ制御レジスタを 0 に書き戻して、次回の割り込み/報告を許可する。
    void clearStatus() {
        try {
            i2c_.write8(COORDINATE_INFO_REG, 0x00);
        } catch (...) {}
    }

private:
    I2C       i2c_;
    GpioLine  rst_;
    GpioLine  int_in_;
    bool      use_events_;
    std::atomic<bool> running_;
    std::thread       th_;
    std::atomic<int>  last_x_, last_y_;
};

} // namespace gt911

// -------------------------------------------------------------
// main
//   1) I2C をスキャンして GT911 のアドレスを推定(ログ出力)
//   2) ST7796 を初期化し、6色バーを描画して表示確認
//   3) GT911 を初期化(割り込み or ポーリング)して座標取得開始
//   4) 50ms 周期ループ: タッチ中は青点を描画、未タッチ時はハートビート矩形を走らせる
//      標準出力には座標を出力。
// -------------------------------------------------------------
int main() {
    try {
        // 1) GT911タッチコントローラのI2Cアドレスを設定
        uint8_t addr{0x14};

        // 2) LCD初期化＆クリア
        st7796::Display lcd;
        lcd.clear(0xFFFF); // white

        // 2.5) テストパターン（6色の横バー）で表示確認
        {
            const uint16_t colors[6] = {0xF800, 0x07E0, 0x001F, 0xFFE0, 0x07FF, 0xF81F};
            const int band = st7796::kHeight / 6;
            for (int i = 0; i < 6; ++i) {
                int y0 = i * band;
                int y1 = (i == 5) ? (st7796::kHeight - 1) : (y0 + band - 1);
                lcd.drawFilledRect(0, y0, st7796::kWidth - 1, y1, colors[i]);
            }
        }

        // 3) タッチ初期化（割り込み or ポーリング）
        gt911::Touch touch(addr);

        // 4) メインループ（タッチ座標を点描／タッチ無しでもハートビート描画）
        int t = 0;
        while (true) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            auto [x, y] = touch.lastXY();
            if (x >= 0 && y >= 0) {
                // タッチ点の可視化（青い点）
                lcd.drawFilledRect(x, y, x + 5, y + 5, 0x001F);
                std::cout << "Coordinate x=" << x << " y=" << y << "\n";
            } else {
                // ハートビート（左→右に走る白い小矩形）
                int bx = (t % st7796::kWidth);
                lcd.drawFilledRect(bx, 0, std::min(bx + 10, st7796::kWidth - 1), 10, 0xFFFF);
                t += 6;
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}