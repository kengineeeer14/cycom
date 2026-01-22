#ifndef CYCOM_HAL_INTERFACE_I_I2C_H_
#define CYCOM_HAL_INTERFACE_I_I2C_H_

#include <cstdint>
#include <cstddef>
#include <vector>

namespace hal {

/**
 * @brief I2C通信の抽象インターフェース
 * 
 * ハードウェアに依存しないI2Cバス通信のインターフェース。
 * テスト時にはモック実装を、本番環境では実ハードウェア実装を使用する。
 */
class II2c {
public:
    virtual ~II2c() = default;

    /**
     * @brief I2Cバスに1バイト書き込む
     * 
     * @param addr スレーブデバイスのI2Cアドレス
     * @param reg レジスタアドレス
     * @param value 書き込む値
     * @return true 成功
     * @return false 失敗
     */
    virtual bool WriteByte(uint8_t addr, uint8_t reg, uint8_t value) = 0;

    /**
     * @brief I2Cバスから1バイト読み取る
     * 
     * @param addr スレーブデバイスのI2Cアドレス
     * @param reg レジスタアドレス
     * @param value 読み取った値を格納する変数への参照
     * @return true 成功
     * @return false 失敗
     */
    virtual bool ReadByte(uint8_t addr, uint8_t reg, uint8_t& value) = 0;

    /**
     * @brief I2Cバスから複数バイト読み取る
     * 
     * @param addr スレーブデバイスのI2Cアドレス
     * @param reg 開始レジスタアドレス
     * @param buffer 読み取ったデータを格納するバッファ
     * @param len 読み取るバイト数
     * @return true 成功
     * @return false 失敗
     */
    virtual bool ReadBytes(uint8_t addr, uint8_t reg, uint8_t* buffer, size_t len) = 0;

    /**
     * @brief I2Cバスに複数バイト書き込む
     * 
     * @param addr スレーブデバイスのI2Cアドレス
     * @param reg 開始レジスタアドレス
     * @param data 書き込むデータの先頭ポインタ
     * @param len 書き込むバイト数
     * @return true 成功
     * @return false 失敗
     */
    virtual bool WriteBytes(uint8_t addr, uint8_t reg, const uint8_t* data, size_t len) = 0;

    /**
     * @brief 16ビットレジスタアドレスを使用してI2Cバスから読み取る
     * 
     * @param addr スレーブデバイスのI2Cアドレス
     * @param reg 16ビットレジスタアドレス
     * @param buffer 読み取ったデータを格納するバッファ
     * @param len 読み取るバイト数
     * @return true 成功
     * @return false 失敗
     */
    virtual bool Read16(uint8_t addr, uint16_t reg, uint8_t* buffer, size_t len) = 0;

    /**
     * @brief 16ビットレジスタアドレスを使用してI2Cバスに書き込む
     * 
     * @param addr スレーブデバイスのI2Cアドレス
     * @param reg 16ビットレジスタアドレス
     * @param data 書き込むデータの先頭ポインタ
     * @param len 書き込むバイト数
     * @return true 成功
     * @return false 失敗
     */
    virtual bool Write16(uint8_t addr, uint16_t reg, const uint8_t* data, size_t len) = 0;
};

}  // namespace hal

#endif  // CYCOM_HAL_INTERFACE_I_I2C_H_
