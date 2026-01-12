#ifndef SENSOR_SENSOR_MANAGER_H
#define SENSOR_SENSOR_MANAGER_H

#include <atomic>
#include <thread>
#include "sensor/gps/gps_l76k.h"

namespace sensor {

/**
 * @brief センサーデータ取得を管理するクラス（Touch / Logger クラスと同じパターン）
 * 
 * コンストラクタでセンサー読み取りスレッドを自動起動し、デストラクタで安全に停止する。
 * 現在はGPS（UART経由）のみサポート。将来的に他のセンサー（I2C、SPI等）も追加可能。
 */
class SensorManager {
public:
    /**
     * @brief SensorManager を初期化し、センサー読み取りスレッドを自動起動する
     * 
     * @param uart_fd UART ファイルディスクリプタ（GPS通信用）
     * @param gps GPS データを格納するオブジェクトへの参照
     */
    SensorManager(int uart_fd, L76k& gps);

    /**
     * @brief センサー読み取りスレッドを安全に停止させる
     */
    ~SensorManager();

private:
    void Start();
    void Stop();
    
    /**
     * @brief センサーデータ読み取りループ（UART経由でGPSデータを受信しパース）
     */
    void SensorLoop();

    int uart_fd_;
    L76k& gps_;
    std::thread th_;
    std::atomic<bool> running_{false};
};

} // namespace sensor

#endif // SENSOR_SENSOR_MANAGER_H
