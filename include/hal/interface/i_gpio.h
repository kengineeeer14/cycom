#ifndef CYCOM_HAL_INTERFACE_I_GPIO_H_
#define CYCOM_HAL_INTERFACE_I_GPIO_H_

#include <ctime>

namespace hal {

/**
 * @brief GPIO操作の抽象インターフェース
 * 
 * ハードウェアに依存しないGPIOピン制御のインターフェース。
 * テスト時にはモック実装を、本番環境では実ハードウェア実装を使用する。
 */
class IGpio {
public:
    virtual ~IGpio() = default;

    /**
     * @brief GPIOピンに値を設定する
     * 
     * @param value 設定する値 (0=Low, 1=High)
     */
    virtual void Set(int value) = 0;

    /**
     * @brief GPIOピンの現在の値を取得する
     * 
     * @return int 現在の値 (0=Low, 1=High)
     */
    virtual int Get() = 0;

    /**
     * @brief 立ち上がりエッジ（Low→High）イベント検出を有効化する
     */
    virtual void RequestRisingEdge() = 0;

    /**
     * @brief 立ち下がりエッジ（High→Low）イベント検出を有効化する
     */
    virtual void RequestFallingEdge() = 0;

    /**
     * @brief GPIOイベントを指定時間待機する
     * 
     * @param timeout_sec タイムアウト時間（秒）
     * @return true イベントが発生した
     * @return false タイムアウトした
     */
    virtual bool WaitForEvent(int timeout_sec) = 0;
};

}  // namespace hal

#endif  // CYCOM_HAL_INTERFACE_I_GPIO_H_
