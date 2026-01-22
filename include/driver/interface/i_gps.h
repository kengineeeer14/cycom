#ifndef CYCOM_DRIVER_INTERFACE_I_GPS_H_
#define CYCOM_DRIVER_INTERFACE_I_GPS_H_

#include <string>
#include <cstdint>

namespace driver {

/**
 * @brief GPS位置情報データ
 */
struct GpsData {
    double latitude;        // 緯度
    double longitude;       // 経度
    double altitude;        // 高度
    double speed_kmh;       // 速度[km/h]
    uint8_t hour;           // UTC時刻: 時
    uint8_t minute;         // UTC時刻: 分
    double second;          // UTC時刻: 秒
    bool valid;             // データが有効か
};

/**
 * @brief GPSドライバの抽象インターフェース
 * 
 * GPSデバイスに依存しない位置情報取得のインターフェース。
 * テスト時にはモック実装を、本番環境では実デバイス実装を使用する。
 */
class IGps {
public:
    virtual ~IGps() = default;

    /**
     * @brief 現在のGPSデータを取得する
     * 
     * @return GpsData GPS位置情報
     */
    virtual GpsData GetData() = 0;

    /**
     * @brief GPSデータが有効か確認する
     * 
     * @return true 有効なデータがある
     * @return false 無効または未取得
     */
    virtual bool IsValid() = 0;

    /**
     * @brief UARTからデータを読み取りパースする
     * 
     * @param fd UARTファイルディスクリプタ
     */
    virtual void ReadAndParse(int fd) = 0;
};

}  // namespace driver

#endif  // CYCOM_DRIVER_INTERFACE_I_GPS_H_
