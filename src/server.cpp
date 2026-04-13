#include "server.h"
#include <zmq.hpp>
#include <fstream>
#include <iostream>
#include <libpq-fe.h>
#include "utils.h"

void run_server(Location* loc, std::mutex* mtx) {
    std::ofstream F("visual.json");

    PGconn *conn = PQconnectdb("host=localhost port=5433 dbname=mobile_data user=postgres password=Lunahod2007");
    if (PQstatus(conn) != CONNECTION_OK) {
        std::cerr << "DB ERROR: " << PQerrorMessage(conn) << std::endl;
        return;
    }

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://0.0.0.0:5551");

    std::cout << "Сервер запущен...\n";

    int count = 0;

    while (true) {
        zmq::message_t request;
        if (!socket.recv(request, zmq::recv_flags::none)) {
            std::cout << "Ошибка получения данных\n";
            continue;
        }

        std::string data(static_cast<char*>(request.data()), request.size());

        try {
            json j = json::parse(data);

            std::cout << "Lat: " << j.value("latitude", 0.0) 
                      << " Lon: " << j.value("longitude", 0.0) << std::endl;

            Location new_loc{};
            new_loc.latitude     = j.value("latitude", 0.0);
            new_loc.longitude    = j.value("longitude", 0.0);
            new_loc.altitude     = j.value("altitude", 0.0);
            new_loc.accuracy     = j.value("accuracy", 0.0);
            new_loc.current_time = j.value("time", 0LL);
            new_loc.traffic_rx   = j.value("rx", 0LL);
            new_loc.traffic_tx   = j.value("tx", 0LL);

            double raw_t = new_loc.current_time / 1000.0;
            if (start_time < 0 && raw_t > 0) start_time = raw_t;
            double t = raw_t - start_time;

            {
                std::lock_guard<std::mutex> lock(*mtx);
                updateHistories(j, t, new_loc);

                if (j.contains("cell")) {
                    new_loc.cell_info = j["cell"].dump();
                    *loc = new_loc;
                }
            }
            std::string lat_s  = std::to_string(new_loc.latitude);
            std::string lon_s  = std::to_string(new_loc.longitude);
            std::string alt_s  = std::to_string(new_loc.altitude);
            std::string acc_s  = std::to_string(new_loc.accuracy);
            std::string time_s = std::to_string(new_loc.current_time);
            std::string rx_s   = std::to_string(new_loc.traffic_rx);
            std::string tx_s   = std::to_string(new_loc.traffic_tx);
            std::string rsrp_s = std::to_string(new_loc.rsrp);
            std::string rsrq_s = std::to_string(new_loc.rsrq);
            std::string rssi_s = std::to_string(new_loc.rssi);
            std::string sinr_s = (new_loc.sinr == std::numeric_limits<int>::min()) 
                               ? "NULL" : std::to_string(new_loc.sinr);

            const char* paramValues[12] = {
                lat_s.c_str(), lon_s.c_str(), alt_s.c_str(), acc_s.c_str(),
                time_s.c_str(), new_loc.cell_info.c_str(),
                rx_s.c_str(), tx_s.c_str(), rsrp_s.c_str(),
                rsrq_s.c_str(), rssi_s.c_str(), sinr_s.c_str()
            };

            PGresult* res = PQexecParams(conn,
                "INSERT INTO device_data "
                "(latitude, longitude, altitude, accuracy, timestamp, cell_info, "
                "traffic_rx, traffic_tx, rsrp, rsrq, rssi, sinr) "
                "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12)",
                12, nullptr, paramValues, nullptr, nullptr, 0);

            if (PQresultStatus(res) == PGRES_COMMAND_OK) {
                std::cout << "INSERT OK" << std::endl;
            } else {
                std::cerr << "INSERT ERROR: " << PQerrorMessage(conn) << std::endl;
            }
            PQclear(res);

            F << j.dump() << std::endl;
            F.flush();

            count++;
            std::cout << "RECEIVED #" << count 
                      << " | PCI count: " << rsrp_histories.size() << std::endl;

        } catch (const std::exception& e) {
            std::cerr << "JSON ERROR: " << e.what() << std::endl;
        }

        zmq::message_t reply(2);
        memcpy(reply.data(), "OK", 2);
        socket.send(reply, zmq::send_flags::none);
    }

    PQfinish(conn);
}