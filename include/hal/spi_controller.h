#ifndef HAL_SPI_CONTROLLER_H_
#define HAL_SPI_CONTROLLER_H_

#include <cstdint>
#include <cstddef>

namespace hal {

class SPI {
public:
    /**
     * @brief 指定された SPI デバイスファイル（例: /dev/spidev0.0）をオープンし、モード・ビット幅・クロック速度などの通信パラメータを設定するコンストラクタ．
     * 
     * @param[in] dev 使用するSPIデバイスファイルのパス（例: /dev/spidev0.0）。
     * @param[in] speed_hz 通信クロック周波数[Hz]．
     * @param[in] mode SPIモード（0〜3のいずれか）．
     * @param[in] bits 1ワードあたりのビット数．
     */
    explicit SPI(const char* dev,
                 const uint32_t &speed_hz = 40000000,
                 const uint8_t &mode = 0,
                 const uint8_t &bits = 8);

    // コピー禁止、ムーブのみ許可
    SPI(const SPI&) = delete;
    SPI& operator=(const SPI&) = delete;

    /**
     * @brief fd_にムーブ元のfd_を代入する．（これにより新しいオブジェクトがSPIデバイスを操作できるようになる）
     * 
     * @param other ムーブ元の SPI オブジェクト。
     */
    SPI(SPI&& other) noexcept;

    /**
     * @brief 既存の SPI オブジェクトに、別の SPI オブジェクトのリソース（ファイルディスクリプタ）を移譲
     * 
     * @param other ムーブ元の SPI オブジェクト
     * @return SPI& 代入後の自分自身（オブジェクト本体）
     */
    SPI& operator=(SPI&& other) noexcept;

    // 指定バッファを一括送信。送信バイト数が一致しない場合は例外。
    /**
     * @brief 開かれている SPI デバイスに対して、指定されたバイト列を送信する
     * 
     * @param data data送信したいバイト列の先頭アドレス
     * @param len 送信するバイト数
     */
    void WriteBytes(const uint8_t* data, const size_t &len);

    /**
     * @brief オブジェクトが破棄されるときに、SPI デバイスのファイルディスクリプタを自動的に閉じる
     * 
     */
    ~SPI();

private:
    int fd_;
};

}  // namespace hal

#endif  // HAL_SPI_CONTROLLER_H_