#include "sensors/uart/L76X.h"
#include <iostream>
#include <cstring>

static const char HEX_TABLE[16] = {'0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'};
static constexpr double pi = 3.14159265358979324;
static constexpr double a = 6378245.0;
static constexpr double ee = 0.00669342162296594323;
static constexpr double x_pi = pi * 3000.0 / 180.0;

void L76X::SendCommand(const std::string& data) {
    if (data.empty()) return;

    char check = data[1];
    for (size_t i = 2; i < data.length(); ++i) {
        check ^= data[i];
    }

    char checksum[3] = {
        HEX_TABLE[(check >> 4) & 0x0F],
        HEX_TABLE[check & 0x0F],
        '\0'
    };

    DevUartSendString(data.c_str());
    DevUartSendByte('*');
    DevUartSendString(checksum);
    DevUartSendByte('\r');
    DevUartSendByte('\n');
    DevDelayMs(200);
}

GNRMC L76X::GetGNRMC() {
    DevUartReceiveString(buffer_, BUFFSIZE);
    std::cout << buffer_ << "\r\n";

    gps_ = {};
    UWORD add = 0;
    while (add < BUFFSIZE - 71) {
        if (buffer_[add] == '$' && buffer_[add+1] == 'G' &&
            (buffer_[add+2] == 'N' || buffer_[add+2] == 'P') &&
            buffer_[add+3] == 'R' && buffer_[add+4] == 'M' && buffer_[add+5] == 'C') {
            
            int x = 0, y = 0, z = 0, i = 0;
            UDOUBLE Time = 0;
            long double latitude = 0, longitude = 0;
            double times = 1.0;

            for (z = 0; x < 12; ++z) {
                if (buffer_[add+z] == '\0') break;
                if (buffer_[add+z] == ',') {
                    x++;
                    switch (x) {
                        case 1: { // 時刻
                            Time = 0;
                            for (i = 0; buffer_[add+z+i+1] != '.'; ++i) {
                                if (buffer_[add+z+i+1] == ',' || buffer_[add+z+i+1] == '\0') break;
                                Time = (buffer_[add+z+i+1] - '0') + Time * 10;
                            }
                            gps_.Time_H = (Time / 10000 + 8) % 24;
                            gps_.Time_M = (Time / 100) % 100;
                            gps_.Time_S = Time % 100;
                            break;
                        }
                        case 2:
                            gps_.Status = (buffer_[add+z+1] == 'A') ? 1 : 0;
                            break;
                        case 3: { // 緯度
                            latitude = 0;
                            for (i = 0; buffer_[add+z+i+1] != ','; ++i) {
                                if (buffer_[add+z+i+1] == '\0') break;
                                if (buffer_[add+z+i+1] == '.') { y = i; continue; }
                                latitude = (buffer_[add+z+i+1] - '0') + latitude * 10;
                            }
                            times = 1.0;
                            while (i >= y) { times *= 10.0; i--; }
                            gps_.Lat = static_cast<double>(latitude) / times;
                            break;
                        }
                        case 4:
                            gps_.Lat_area = buffer_[add+z+1];
                            break;
                        case 5: { // 経度
                            longitude = 0;
                            for (i = 0; buffer_[add+z+i+1] != ','; ++i) {
                                if (buffer_[add+z+i+1] == '\0') break;
                                if (buffer_[add+z+i+1] == '.') { y = i; continue; }
                                longitude = (buffer_[add+z+i+1] - '0') + longitude * 10;
                            }
                            times = 1.0;
                            while (i >= y) { times *= 10.0; i--; }
                            gps_.Lon = static_cast<double>(longitude) / times;
                            break;
                        }
                        case 6:
                            gps_.Lon_area = buffer_[add+z+1];
                            break;
                        default:
                            break;
                    }
                }
            }
            break;
        }
        add++;
    }
    return gps_;
}

Coordinates L76X::GetBaiduCoordinates() {
    Coordinates temp;
    temp.Lat = static_cast<int>(gps_.Lat) + (gps_.Lat - static_cast<int>(gps_.Lat)) * 100 / 60;
    temp.Lon = static_cast<int>(gps_.Lon) + (gps_.Lon - static_cast<int>(gps_.Lon)) * 100 / 60;
    return BdEncrypt(Transform(temp));
}

Coordinates L76X::GetGoogleCoordinates() {
    Coordinates temp;
    gps_.Lat = static_cast<int>(gps_.Lat) + (gps_.Lat - static_cast<int>(gps_.Lat)) * 100 / 60;
    gps_.Lon = static_cast<int>(gps_.Lon) + (gps_.Lon - static_cast<int>(gps_.Lon)) * 100 / 60;
    temp = Transform({gps_.Lon, gps_.Lat});
    return temp;
}

double L76X::TransformLat(double x, double y) {
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(std::abs(x));
    ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
    ret += (20.0 * sin(y * pi) + 40.0 * sin(y / 3.0 * pi)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * pi) + 320.0 * sin(y * pi / 30.0)) * 2.0 / 3.0;
    return ret;
}

double L76X::TransformLon(double x, double y) {
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(std::abs(x));
    ret += (20.0 * sin(6.0 * x * pi) + 20.0 * sin(2.0 * x * pi)) * 2.0 / 3.0;
    ret += (20.0 * sin(x * pi) + 40.0 * sin(x / 3.0 * pi)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * pi) + 300.0 * sin(x / 30.0 * pi)) * 2.0 / 3.0;
    return ret;
}

Coordinates L76X::Transform(Coordinates gps) {
    Coordinates gg;
    double dLat = TransformLat(gps.Lon - 105.0, gps.Lat - 35.0);
    double dLon = TransformLon(gps.Lon - 105.0, gps.Lat - 35.0);
    double radLat = gps.Lat / 180.0 * pi;
    double magic = sin(radLat);
    magic = 1 - ee * magic * magic;
    double sqrtMagic = sqrt(magic);
    dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * pi);
    dLon = (dLon * 180.0) / (a / sqrtMagic * cos(radLat) * pi);
    gg.Lat = gps.Lat + dLat;
    gg.Lon = gps.Lon + dLon;
    return gg;
}

Coordinates L76X::BdEncrypt(Coordinates gg) {
    Coordinates bd;
    double x = gg.Lon, y = gg.Lat;
    double z = sqrt(x * x + y * y) + 0.00002 * sin(y * x_pi);
    double theta = atan2(y, x) + 0.000003 * cos(x * x_pi);
    bd.Lon = z * cos(theta) + 0.0065;
    bd.Lat = z * sin(theta) + 0.006;
    return bd;
}
