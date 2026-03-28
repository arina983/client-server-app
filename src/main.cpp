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

using namespace std;
using json = nlohmann::json;

void showField(const string& text, const regex& r, const char* label, const char* unit);
void printCellInfoPretty(const string& cell);
void parseCellInfo(const string& cell);

vector<double> rsrp_history;
vector<double> rsrq_history;
vector<double> rssi_history;
vector<double> time_history;

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
};
void parseCellInfo(const string& cell) {

    if (cell.find("LTE") != string::npos) {
        ImGui::TextColored(ImVec4(0,1,0,1), "LTE (4G)");
        showField(cell, regex("rsrp=(-?[0-9]+)"), "RSRP", "dBm");
        showField(cell, regex("rsrq=(-?[0-9]+)"), "RSRQ", "dB");
        showField(cell, regex("rssi=(-?[0-9]+)"), "RSSI", "dBm");
    }
}
void thread1(location *loc, mutex *mtx){

    ofstream F("visual.json");

    PGconn *conn = PQconnectdb("host=localhost port=5433 dbname=mobile_data user=postgres password=Lunahod2007");

    if (PQstatus(conn) != CONNECTION_OK) {
        cerr << "DB ERROR: " << PQerrorMessage(conn) << endl;
        return;
    }

    zmq::context_t context(1);
    zmq::socket_t socket(context, ZMQ_REP);
    socket.bind("tcp://0.0.0.0:5551");

    cout << "Сервер запущен и ожидает подключения...\n";

    int count = 0;

    while (true){
        zmq::message_t request;
        auto result = socket.recv(request, zmq::recv_flags::none);
        if (!result) {
            std::cout << "Ошибка получения данных\n";
            continue;
        }

        string data(static_cast<char*>(request.data()), request.size());

        try {
            json j = json::parse(data);

            cout << "Latitude:  " << j.value("latitude", 0.0) << endl;
            cout << "Longitude: " << j.value("longitude", 0.0) << endl;
            cout << "Altitude:  " << j.value("altitude", 0.0) << endl;
            cout << "Accuracy:  " << j.value("accuracy", 0.0) << endl;
            cout << "Time:      " << j.value("time", 0LL) << endl;

            cout << "RX: " << j.value("rx", 0LL) << " bytes" << endl;
            cout << "TX: " << j.value("tx", 0LL) << " bytes" << endl;

            string cell = j.value("cell", "");
            printCellInfoPretty(cell);

            cout << "====================\n";

            location new_loc;

            new_loc.longitude = j.value("longitude", 0.0);
            new_loc.latitude  = j.value("latitude", 0.0);
            new_loc.altitude  = j.value("altitude", 0.0);
            new_loc.accuracy  = j.value("accuracy", 0.0);
            new_loc.current_time = j.value("time", 0LL);
            new_loc.cell_info = j.value("cell", "");
            new_loc.traffic_rx = j.value("rx", 0LL);
            new_loc.traffic_tx = j.value("tx", 0LL);

            // --- сигнал ---
            smatch match;

            if (regex_search(new_loc.cell_info, match, regex("rsrp=(-?[0-9]+)")))
                new_loc.rsrp = stoi(match[1]);

            if (regex_search(new_loc.cell_info, match, regex("rsrq=(-?[0-9]+)")))
                new_loc.rsrq = stoi(match[1]);

            if (regex_search(new_loc.cell_info, match, regex("rssi=(-?[0-9]+)")))
                new_loc.rssi = stoi(match[1]);

            cout << "RSRP: " << new_loc.rsrp << " dBm" << endl;
            cout << "RSRQ: " << new_loc.rsrq << " dB" << endl;
            cout << "RSSI: " << new_loc.rssi << " dBm" << endl;
            {
                lock_guard<mutex> lock(*mtx);
                *loc = new_loc;
            }
            string lat_s = to_string(new_loc.latitude);
            string lon_s = to_string(new_loc.longitude);
            string alt_s = to_string(new_loc.altitude);
            string acc_s = to_string(new_loc.accuracy);
            string time_s = to_string(new_loc.current_time);
            string rx_s = to_string(new_loc.traffic_rx);
            string tx_s = to_string(new_loc.traffic_tx);
            string rsrp_s = to_string(new_loc.rsrp);
            string rsrq_s = to_string(new_loc.rsrq);
            string rssi_s = to_string(new_loc.rssi);

            const char* paramValues[11] = {
                lat_s.c_str(), lon_s.c_str(), alt_s.c_str(), acc_s.c_str(),
                time_s.c_str(), new_loc.cell_info.c_str(),
                rx_s.c_str(), tx_s.c_str(),
                rsrp_s.c_str(), rsrq_s.c_str(), rssi_s.c_str()
            };

            PGresult* res = PQexecParams(
                conn,
                "INSERT INTO device_data "
                "(latitude, longitude, altitude, accuracy, timestamp, cell_info, "
                " traffic_rx, traffic_tx, rsrp, rsrq, rssi) "
                "VALUES ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11)",
                11, NULL, paramValues, NULL, NULL, 0
            );

            if (PQresultStatus(res) == PGRES_COMMAND_OK) {
                cout << "✓ INSERT OK (rows affected: " 
                    << PQcmdTuples(res) << ")" << endl;
            } else {
                cerr << "✗ INSERT ERROR: " << PQresultErrorMessage(res) << endl;
                cerr << "Command was: INSERT INTO device_data ..." << endl;
            }

            PQclear(res);

            // --- файл ---
            F << j.dump() << endl;
            F.flush();

            count++;
            cout << "RECEIVED: " << count << endl;

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

    cout << "=== CELL INFO ===\n";

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
void run_gui(location *loc, mutex *mtx){

    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Monitor", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1200, 900, SDL_WINDOW_OPENGL);
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

            double t = local_copy.current_time / 1000.0;

            time_history.push_back(t);
            rsrp_history.push_back(local_copy.rsrp);
            rsrq_history.push_back(local_copy.rsrq);
            rssi_history.push_back(local_copy.rssi);
        }

        ImGui::Begin("Cell Info");

        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Location");
        ImGui::Text("Longitude: %.6f", local_copy.longitude);
        ImGui::Text("Latitude:  %.6f", local_copy.latitude);
        
        ImGui::Text("Altitude:        %.2f", local_copy.altitude);
        ImGui::Text("Accuracy:      %.2f ", local_copy.accuracy);
        ImGui::Text("Current_time:         %lld", local_copy.current_time);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Traffic");
        ImGui::Text("RX:  %lld byte  (%.2f mb)", 
                    local_copy.traffic_rx, local_copy.traffic_rx / 1048576.0);
        ImGui::Text("TX:  %lld byte  (%.2f mb)", 
                    local_copy.traffic_tx, local_copy.traffic_tx / 1048576.0);

        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Signal LTE");

        ImVec4 color_rsrp = (local_copy.rsrp > -90) ? ImVec4(0.0f, 1.0f, 0.0f, 1.0f) :
                            (local_copy.rsrp > -110) ? ImVec4(1.0f, 1.0f, 0.0f, 1.0f) :
                                                       ImVec4(1.0f, 0.0f, 0.0f, 1.0f);

        ImGui::TextColored(color_rsrp, "RSRP:  %d dBm", local_copy.rsrp);
        ImGui::TextColored(color_rsrp, "RSRQ:  %d dB",  local_copy.rsrq);
        ImGui::TextColored(color_rsrp, "RSSI:  %d dBm", local_copy.rssi);

        ImGui::Separator();
        
        parseCellInfo(local_copy.cell_info);

        ImGui::Separator();

        if (!rsrp_history.empty()) {
            if (ImPlot::BeginPlot("RSRP", ImVec2(-1, 250))) {
                ImPlot::SetupAxis(ImAxis_X1, "Time (second)");
                ImPlot::SetupAxis(ImAxis_Y1, "RSRP (dBm)");
                ImPlot::PlotLine("RSRP", time_history.data(), rsrp_history.data(), rsrp_history.size());
                ImPlot::EndPlot();
            }

            if (ImPlot::BeginPlot("RSRQ", ImVec2(-1, 200))) {
                ImPlot::SetupAxis(ImAxis_X1, "Time (second)");
                ImPlot::SetupAxis(ImAxis_Y1, "RSRQ (dB)");
                ImPlot::PlotLine("RSRQ", time_history.data(), rsrq_history.data(), rsrq_history.size());
                ImPlot::EndPlot();
            }

            if (ImPlot::BeginPlot("RSSI", ImVec2(-1, 200))) {
                ImPlot::SetupAxis(ImAxis_X1, "Время (сек)");
                ImPlot::SetupAxis(ImAxis_Y1, "RSSI (dBm)");
                ImPlot::PlotLine("RSSI", time_history.data(), rssi_history.data(), rssi_history.size());
                ImPlot::EndPlot();
            }
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