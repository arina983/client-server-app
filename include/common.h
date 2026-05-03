#pragma once

#include <string>
#include <map>
#include <vector>
#include <mutex>
#include <limits>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

extern double start_time;

struct Location {
    double latitude = 0.0;
    double longitude = 0.0;
    double altitude = 0.0;
    double accuracy = 0.0;
    long long current_time = 0;
    std::string cell_info;
    long long traffic_rx = 0;
    long long traffic_tx = 0;
    int rsrp = 0;
    int rsrq = 0;
    int rssi = 0;
    int sinr = std::numeric_limits<int>::min();
};

extern std::map<int, std::vector<double>> rsrp_histories;
extern std::map<int, std::vector<double>> rsrq_histories;
extern std::map<int, std::vector<double>> rssi_histories;
extern std::map<int, std::vector<double>> sinr_histories;
extern std::map<int, std::vector<double>> time_histories;
extern std::mutex data_mutex;