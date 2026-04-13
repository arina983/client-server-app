#include "common.h"
#include "server.h"
#include "gui.h"
#include <thread>

double start_time = -1.0;

std::map<int, std::vector<double>> rsrp_histories;
std::map<int, std::vector<double>> rsrq_histories;
std::map<int, std::vector<double>> rssi_histories;
std::map<int, std::vector<double>> sinr_histories;
std::map<int, std::vector<double>> time_histories;

std::mutex data_mutex;

int main() {
    static Location loc{};
    
    std::thread t1(run_server, &loc, &data_mutex);
    std::thread t2(run_gui, &loc, &data_mutex);

    t1.join();
    t2.join();

    return 0;
}