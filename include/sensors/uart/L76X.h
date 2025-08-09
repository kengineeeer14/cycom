#ifndef L76X_H
#define L76X_H

#include "gpio/DEV_Config.h"
#include <string>
#include <cmath>

constexpr int BUFFSIZE = 800;

struct GNRMC {
    double Lon;
    double Lat;
    UBYTE Lon_area;
    UBYTE Lat_area;
    UBYTE Time_H;
    UBYTE Time_M;
    UBYTE Time_S;
    UBYTE Status;
};

struct Coordinates {
    double Lon;
    double Lat;
};

class L76X {
public:
    /**
     * @brief GPSモジュールにコマンドを送信．NMEA準拠のコマンドとチェックサムを送る．
     * 
     * @param cmd 
     */
    void SendCommand(const std::string& cmd);

    char Test();

    /**
     * @brief $GNRMC または $GPRMC センテンスをパースし、GPS時刻・位置・状態を抽出．
     * 
     * @return GNRMC 
     */
    GNRMC GetGNRMC();

    /**
     * @brief GPSデータを10進度形式に変換し、GCJ-02 → BD-09（百度地図）座標を返す
     * 
     * @return Coordinates 
     */
    Coordinates GetBaiduCoordinates();
    
    /**
     * @brief GPSデータを10進度形式に変換し、WGS-84 → GCJ-02（Google中国版対応）座標を返す
     * 
     * @return Coordinates 
     */
    Coordinates GetGoogleCoordinates();

private:
    char buffer_[BUFFSIZE] = {0};
    GNRMC gps_;
    UartConfig uartconfig_;

    /**
     * @brief WGS-84(GPSが使うグローバルな座標系)からGCJ-02(中国政府が定めた火星座標)に
     * 変換する補正関数の一部で，緯度方向の補正を計算する．
     * NOTE:中国以外では不要
     * 
     * @param[in] x 基準点からの経度の差分
     * @param[in] y 基準点からの緯度の差分
     * @return double 
     */
    static double TransformLat(double x, double y);

    /**
     * @brief WGS-84(GPSが使うグローバルな座標系)からGCJ-02(中国政府が定めた火星座標)に
     * 変換する補正関数の一部で，経度方向の補正を計算する．
     * NOTE:中国以外では不要
     * 
     * @param[in] x 基準点からの経度の差分
     * @param[in] y 基準点からの緯度の差分
     * @return double 
     */
    static double TransformLon(double x, double y);

    /**
     * @brief GPSモジュール（WGS-84）から得た座標を、
     * 中国の地図サービス（百度地図・高徳地図など）と一致する座標（GCJ-02）に変換
     * 
     * @param[in] gps WGS-84の緯度と経度
     * @return Coordinates GCJ-02の緯度と経度
     */
    static Coordinates Transform(Coordinates gps);

    /**
     * @brief 中国の**GCJ-02座標（火星座標）を、百度（Baidu）独自の BD-09 座標系に変換
     * 
     * @param[in] gg GCJ-02の緯度と経度
     * @return Coordinates BD-09の緯度と経度
     */
    static Coordinates BdEncrypt(Coordinates gg);
};

#endif
