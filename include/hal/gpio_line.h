#ifndef CYCOM_HAL_GPIO_LINE_H_
#define CYCOM_HAL_GPIO_LINE_H_

#include <gpiod.h>
#include <string>
#include <stdexcept>
#include <ctime>    // timespec

namespace gpio {

// 単一GPIOラインのRAIIラッパ（libgpiod v1）
class GpioLine {
public:
    /**
     * @brief 指定したチップとラインのハンドラを内部変数に格納する．出力ピンを予約し，初期値を設定する．
     * (chip は "/dev/gpiochip0" or "gpiochip0" でもOK)
     * 
     * @param[in] chip GPIO チップの指定。例: "/dev/gpiochip0" または "gpiochip0"
     * @param[in] offset チップ内の line 番号 (GPIO 番号に対応)。例: 17 → GPIO17
     * @param[in] output 出力ピンとして扱うかどうかのフラグ
     * @param[in] initial 出力ピンにする場合の初期値 (0=Low, 1=High)
     */
    GpioLine(const std::string& chip, const unsigned int &offset, const bool &output, const int &initial = 0);

    /**
     * @brief 指定したチップとラインのハンドラを内部変数に格納する．出力ピンを予約し，初期値を設定する．
     * 
     */
    void RequestRisingEdge();

    /**
     * @brief 指定したGPIO lineを入力ピンに設定し，Low→High に変化したときにイベントを発生させるように設定する．
     * 
     */
    void RequestFallingEdge();

    /**
     * @brief 指定した GPIO line にイベント（エッジ変化）が届くのを、指定した時間だけ待機して，結果を返す．
     * 
     * @param[in] timeout_ms イベントを待つ最大時間 [ms]
     * @return int イベントが発生したかどうかを表す整数値
     *（1 → イベントが発生した（GPIO ピンにエッジ変化あり），
     * 0 → タイムアウト（指定時間内に変化なし），
     * -1 → エラー（line が無効、権限不足など））
     */
    int WaitEvent(const int &timeout_ms);

    /**
     * @brief GPIO ピンに発生した次のイベント（立ち上がり／立ち下がりなど）を読み取って、呼び出し元に渡された gpiod_line_event 構造体に格納する
     * 
     * @param[in] ev イベントデータがコピーされるバッファ。 
     */
    void ReadEvent(gpiod_line_event &ev);

    /**
     * @brief GPIO ラインの出力値を設定する
     * 
     * @param[in] value 設定させる値（GPIO の論理レベル）
     */
    void Set(const int &value);

    // コピー/ムーブ禁止（所有明確化）
    GpioLine(const GpioLine&) = delete;
    GpioLine& operator=(const GpioLine&) = delete;
    GpioLine(GpioLine&&) = delete;
    GpioLine& operator=(GpioLine&&) = delete;

    /**
     * @brief Destroy the Gpio Line object
     * 
     */
    ~GpioLine();

private:
    /**
     * @brief 指定された文字列 chip をもとに libgpiod の gpiod_chip 構造体を開く．
     * 
     * @param[in] chip /dev/gpiochip0 のようなパス文字列、または gpiochip0 のようなチップ名。
     * @return 開いた GPIO チップのハンドル（libgpiod の構造体へのポインタ）
     */
    static gpiod_chip* OpenChipFlexible(const std::string& chip);

private:
    gpiod_chip* chip_{nullptr};
    gpiod_line* line_{nullptr};
    bool is_output_{false};
    const char* consumer_ = "cycom";
};

}  // namespace gpio

#endif  // CYCOM_HAL_GPIO_LINE_H_