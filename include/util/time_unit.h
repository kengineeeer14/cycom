#ifndef TIME_UNIT_H
#define TIME_UNIT_H

namespace util {
    class TimeUnit {
        public:
            static constexpr double kMs2Sec{0.001};
            static constexpr double kNs2Ms{0.000001};
            static constexpr int kMs2Ns{1000000};

            /**
             * @brief 時間[ms]のミリ秒部分を取り出す
             * 
             * @param[in] time_sec 時間[ms]
             * @return int 時間のms部分[s]
             */
            static int msWithinMs(const int &time_sec);
    };
}   // namespace util

#endif // TIME_UNIT_H