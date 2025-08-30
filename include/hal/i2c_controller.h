#ifndef I2C_CONTROLLER_H_
#define I2C_CONTROLLER_H_

#include <cstdint>
#include <cstddef>

namespace hal{

// I2C 通信を行うためのヘルパークラス。
//   - /dev/i2c-* デバイスを open して使用する。
//   - 16bit レジスタアドレス付きの read/write を提供。
//   - 8bit 読み書き用のショートカットも用意。
class I2C {
public:
    /**
     * @brief 指定したI2Cデバイスファイルを「読み書きモード」で開き，これから通信する相手を設定する．’
     * 
     * @param[in] dev デバイスファイルのパス（例: "/dev/i2c-1"）
     * @param[in] addr 接続先スレーブデバイスの 7ビットアドレス
     */
    I2C(const char* dev, const uint8_t &addr);

    // 
    /**
     * @brief 16bit レジスタアドレスに任意長データを書き込む。
     * 
     * @param[in] reg 16ビットのレジスタアドレス。「I2Cスレーブデバイス内でどのレジスタに書き込むか」を指定する番号
     * @param[in] data 書き込みたいデータの先頭アドレス（ポインタ）
     * @param[in] len 書き込むデータのバイト数
     */
    void WriteReg16(const uint16_t &reg, const uint8_t* data, const size_t &len);

    /**
     * @brief 接続先スレーブデバイスから読み出したデータを格納する，
     * 
     * @param[in] reg 16ビットのレジスタアドレス。「I2Cスレーブデバイス内でどのレジスタから読み取るか」を指定する番号
     * @param[in] out 読み出すデータを格納するバッファの先頭アドレス（ポインタ）
     * @param[in] len 読み出すデータのバイト数
     */
    void ReadReg16(const uint16_t &reg, uint8_t* out, const size_t &len);

    /**
     * @brief 接続先スレーブデバイスから1バイトのデータを読み出す．
     * 
     * @param[in] reg 16ビットのレジスタアドレス。「I2Cスレーブデバイス内でどのレジスタから読み取るか」を指定する番号
     * @return uint8_t 読み出したデータ
     */
    uint8_t Read8(const uint16_t &reg);

    /**
     * @brief 16bit レジスタアドレスに1バイトのデータを書き込む。
     * 
     * @param[in] reg reg 16ビットのレジスタアドレス。「I2Cスレーブデバイス内でどのレジスタに書き込むか」を指定する番号
     * @param[in] v 書き込みたいデータの先頭アドレス（ポインタ）
     */
    void Write8(const uint16_t &reg, const uint8_t &v);

    /**
     * @brief オープンしていた I2C デバイスファイルを閉じる
     * 
     */
    ~I2C();

private:
    int fd_;
    uint8_t addr_;
};
}   // namespace hal
#endif  // I2C_CONTROLLER_H_