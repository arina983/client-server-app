#include <iostream>
#include "imgui.h"
#include "implot.h"
#include <string>
#include <zmq.hpp>
#include <fstream>
#include <thread>
#include <GL/glew.h>
#include <SDL2/SDL.h>
#include "backends/imgui_impl_opengl3.h"
#include "backends/imgui_impl_sdl2.h"
#include <mutex>
#include <regex>
#include <libpq-fe.h>
#include <nlohmann/json.hpp>
#include <map>
#include <limits>
#include <climits>

using namespace std;
using json = nlohmann::json;

double start_time = -1.0;

void showField(const string& text, const regex& r, const char* label, const char* unit);
void printCellInfoPretty(const string& cell);
void parseCellInfo(const string& cell);

map<int, vector<double>> rsrp_histories;
map<int, vector<double>> rsrq_histories;
map<int, vector<double>> rssi_histories;
map<int, vector<double>> time_histories;
std::map<int, std::vector<double>> sinr_histories;

struct location {
    double latitude = 0;
    double longitude = 0;
    double altitude = 0;
    double accuracy = 0;
    long long current_time = 0;
    string cell_info = "";
    long long traffic_rx = 0;
    long long traffic_tx = 0;
    int rsrp = 0;
    int rsrq = 0;
    int rssi = 0;
    int sinr = std::numeric_limits<int>::min();
};
void parseCellInfo(const string& cell) {

    if (cell.find("LTE") != string::npos) {
        ImGui::TextColored(ImVec4(0,1,0,1), "LTE (4G)");
        showField(cell, regex("rsrp=(-?[0-9]+)"), "RSRP", "dBm");
        showField(cell, regex("rsrq=(-?[0-9]+)"), "RSRQ", "dB");
        showField(cell, regex("rssi=(-?[0-9]+)"), "RSSI", "dBm");
    }
}
void thread1(location *loc, mutex *mtx) {
    ofstream F("visual.json");

    PGconn *conn = PQconnectdb("host=localhost port=5433 dbname=mobile_data user=postgres password=Lunahod2007");
    if (PQstatus(conn) != CONNECTION_OK) {
        cerr << "DB ERROR: " << PQerrorMessage(conn) << endl;
        return;
    }

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://0.0.0.0:5551");

    cout << "Сервер запущен...\n";

    int count = 0;

    while (true) {
        zmq::message_t request;
        if (!socket.recv(request, zmq::recv_flags::none)) {
            cout << "Ошибка получения данных\n";
            continue;
        }

        string data(static_cast<char*>(request.data()), request.size());

        try {
            json j = json::parse(data);

            cout << "Lat: " << j.value("latitude", 0.0) 
                 << " Lon: " << j.value("longitude", 0.0) << endl;

            location new_loc{};
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
            lock_guard<mutex> lock(*mtx);
            if (j.contains("cell") && j["cell"].is_array()) {
                json cell_array = j["cell"];

                for (auto& c : cell_array) {
                    int pci = c.value("pci", -1);
                    if (pci < 0) continue;

                    int rsrp = c.value("rsrp", INT_MIN);
                    int rsrq = c.value("rsrq", INT_MIN);
                    int rssi = c.value("rssi", INT_MIN);
                    int sinr = c.value("sinr", INT_MIN);

                    time_histories[pci].push_back(t);
                    if (rsrp != INT_MIN) rsrp_histories[pci].push_back((double)rsrp);
                    if (rsrq != INT_MIN) rsrq_histories[pci].push_back((double)rsrq);
                    if (rssi != INT_MIN) rssi_histories[pci].push_back((double)rssi);
                    if (sinr != INT_MIN) sinr_histories[pci].push_back((double)sinr);

                    if (new_loc.rsrp == 0 || c.value("isRegistered", false)) {
                        new_loc.rsrp = rsrp;
                        new_loc.rsrq = rsrq;
                        new_loc.rssi = rssi;
                        new_loc.sinr = sinr;
                    }

                    if (time_histories[pci].size() > 3000) {
                        time_histories[pci].erase(time_histories[pci].begin());
                        if (!rsrp_histories[pci].empty()) rsrp_histories[pci].erase(rsrp_histories[pci].begin());
                        if (!rsrq_histories[pci].empty()) rsrq_histories[pci].erase(rsrq_histories[pci].begin());
                        if (!rssi_histories[pci].empty()) rssi_histories[pci].erase(rssi_histories[pci].begin());
                        if (!sinr_histories[pci].empty()) sinr_histories[pci].erase(sinr_histories[pci].begin());
                    }
                }
            }
            
            if (j.contains("cell")) {
                new_loc.cell_info = j["cell"].dump();
                *loc = new_loc;
            }
            string lat_s = to_string(new_loc.latitude);
            string lon_s = to_string(new_loc.longitude);
            string time_s = to_string(new_loc.current_time);
            string rx_s = to_string(new_loc.traffic_rx);
            string tx_s = to_string(new_loc.traffic_tx);
            string rsrp_s = to_string(new_loc.rsrp);
            string rsrq_s = to_string(new_loc.rsrq);
            string rssi_s = to_string(new_loc.rssi);
            string sinr_s  = (new_loc.sinr == std::numeric_limits<int>::min()) ? "NULL" : to_string(new_loc.sinr);

            const char* paramValues[12] = {
                lat_s.c_str(), lon_s.c_str(), to_string(new_loc.altitude).c_str(), 
                to_string(new_loc.accuracy).c_str(), time_s.c_str(), new_loc.cell_info.c_str(),
                rx_s.c_str(), tx_s.c_str(), rsrp_s.c_str(), rsrq_s.c_str(), rssi_s.c_str(), sinr_s.c_str()
            };

            PGresult* res = PQexecParams(conn,
                "INSERT INTO device_data (latitude, longitude, altitude, accuracy, timestamp, cell_info, "
                "traffic_rx, traffic_tx, rsrp, rsrq, rssi, sinr) VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11,$12)",
                11, NULL, paramValues, NULL, NULL, 0);

            if (PQresultStatus(res) == PGRES_COMMAND_OK) {
                cout << "INSERT OK" << endl;
            } else {
                cerr << "INSERT ERROR" << endl;
            }
            PQclear(res);

            F << j.dump() << endl;
            F.flush();

            count++;
            cout << "RECEIVED #" << count << " | PCI count: " << rsrp_histories.size() << endl;

        } catch (exception& e) {
            cerr << "JSON ERROR: " << e.what() << endl;
        }

        zmq::message_t reply(2);
        memcpy(reply.data(), "OK", 2);
        socket.send(reply, zmq::send_flags::none);
    }
}

void showField(const string& text, const regex& r, const char* label, const char* unit = "") {
    smatch match;
    if (regex_search(text, match, r)) {
        if (strlen(unit) > 0)
            ImGui::Text("    %s: %s %s", label, match[1].str().c_str(), unit);
        else
            ImGui::Text("    %s: %s", label, match[1].str().c_str());
    }
}

void printCellInfoPretty(const string& cell) {
    smatch m;

    cout << "CELL INFO \n";

    if (regex_search(cell, m, regex("mMcc=([0-9]+)")))
        cout << "MCC: " << m[1] << endl;

    if (regex_search(cell, m, regex("mMnc=([0-9]+)")))
        cout << "MNC: " << m[1] << endl;

    if (regex_search(cell, m, regex("mCi=([0-9]+)")))
        cout << "Cell ID: " << m[1] << endl;

    if (regex_search(cell, m, regex("mPci=([0-9]+)")))
        cout << "PCI: " << m[1] << endl;

    if (regex_search(cell, m, regex("mTac=([0-9]+)")))
        cout << "TAC: " << m[1] << endl;

    if (regex_search(cell, m, regex("mEarfcn=([0-9]+)")))
        cout << "EARFCN: " << m[1] << endl;

    if (regex_search(cell, m, regex("rsrp=(-?[0-9]+)")))
        cout << "RSRP: " << m[1] << " dBm\n";

    if (regex_search(cell, m, regex("rsrq=(-?[0-9]+)")))
        cout << "RSRQ: " << m[1] << " dB\n";

    if (regex_search(cell, m, regex("rssi=(-?[0-9]+)")))
        cout << "RSSI: " << m[1] << " dBm\n";

    cout << "=================\n";
}
void run_gui(location *loc, mutex *mtx) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1300, 950, SDL_WINDOW_OPENGL);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    static long long last_time = 0;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        location local_copy;
        {
            lock_guard<mutex> lock(*mtx);
            local_copy = *loc;
        }

        if (local_copy.current_time != last_time && local_copy.current_time > 0) {
            last_time = local_copy.current_time;
        }

        ImGui::Begin("Cell Info");

        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Location");
        ImGui::Text("Lon: %.6f   Lat: %.6f", local_copy.longitude, local_copy.latitude);
        ImGui::Text("Time: %lld", local_copy.current_time);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Signal (Serving Cell)");
        ImVec4 color = (local_copy.rsrp > -90) ? ImVec4(0,1,0,1) :
                       (local_copy.rsrp > -110) ? ImVec4(1,1,0,1) : ImVec4(1,0,0,1);

        ImGui::TextColored(color, "RSRP: %d dBm", local_copy.rsrp);
        ImGui::TextColored(color, "RSRQ: %d dB",  local_copy.rsrq);
        ImGui::TextColored(color, "RSSI: %d dBm", local_copy.rssi);

        ImGui::Separator();

        if (ImPlot::BeginPlot("RSRP", ImVec2(-1, 280))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
            ImPlot::SetupAxis(ImAxis_Y1, "RSRP (dBm)");
            lock_guard<mutex> lock(*mtx);
            for (auto& [pci, data] : rsrp_histories) {
                if (!data.empty())
                    ImPlot::PlotLine(("PCI " + to_string(pci)).c_str(), 
                                     time_histories[pci].data(), data.data(), (int)data.size());
            }
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("RSRQ", ImVec2(-1, 220))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
            ImPlot::SetupAxis(ImAxis_Y1, "RSRQ (dB)");
            lock_guard<mutex> lock(*mtx);
            for (auto& [pci, data] : rsrq_histories) {
                if (!data.empty())
                    ImPlot::PlotLine(("PCI " + to_string(pci)).c_str(), 
                                     time_histories[pci].data(), data.data(), (int)data.size());
            }
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("RSSI", ImVec2(-1, 220))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
            ImPlot::SetupAxis(ImAxis_Y1, "RSSI (dBm)");
            lock_guard<mutex> lock(*mtx);
            for (auto& [pci, data] : rssi_histories) {
                if (!data.empty())
                    ImPlot::PlotLine(("PCI " + to_string(pci)).c_str(), 
                                     time_histories[pci].data(), data.data(), (int)data.size());
            }
            ImPlot::EndPlot();
        }
        if (ImPlot::BeginPlot("SINR", ImVec2(-1, 220))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
            ImPlot::SetupAxis(ImAxis_Y1, "SINR (dB)");
            lock_guard<mutex> lock(*mtx);
            for (auto& [pci, data] : sinr_histories) {
                if (!data.empty()) {
                    string label = "PCI " + to_string(pci);
                    ImPlot::PlotLine(label.c_str(), 
                                    time_histories[pci].data(), 
                                    data.data(), 
                                    (int)data.size());
                }
            }
            ImPlot::EndPlot();
        }

        ImGui::End();

        ImGui::Render();
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImPlot::DestroyContext();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
int main() {

    static location loc;
    static mutex mtx;
    thread t1(thread1, &loc, &mtx);
    thread t2(run_gui, &loc, &mtx);
    t1.join();
    t2.join();

    return 0;
}