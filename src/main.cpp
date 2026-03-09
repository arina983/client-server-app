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

using namespace std;

struct location {
    double latitude;
    double longitude;
    double altitude;
    double accuracy;          
    double current_time;
    string cell_info;
    long long traffic_rx;       
    long long traffic_tx;      
    
    location() : latitude(0), longitude(0), altitude(0), accuracy(0), current_time(0), cell_info(""), traffic_rx(0), traffic_tx(0) {}
};

void thread1(location *loc, mutex *mtx){
    ofstream F("visual.json");
    if (!F){
        cout << "Ошибка открытия файла";
        return;
    }
    try {
        zmq::context_t context(1);
        zmq::socket_t socket (context, ZMQ_REP);
        socket.bind("tcp://0.0.0.0:5559");
        int count = 0;
        cout << "Сервер запущен и ожидает подключений"<< endl;

    while (true){
        zmq::message_t request;
        zmq::recv_result_t result = socket.recv(request, zmq::recv_flags::none);
            if (!result) {
                cout << "Ошибка при получении данных или клиент отключился" << endl;
                continue;
            }
        std::string data(static_cast<char*>(request.data()), request.size());
        
        count++;

        size_t first_delim = data.find(" | ");
        size_t second_delim = data.find(" | RX=");
        
        if (first_delim != string::npos && second_delim != string::npos) {
            string coords = data.substr(0, first_delim);
            string cell_info = data.substr(first_delim + 3, second_delim - (first_delim + 3));
            string traffic_str = data.substr(second_delim + 6); 
            
            cout << "Координаты: " << coords << endl;
            cout << "Cell Info: " << cell_info << endl;
            cout << "Трафик: " << traffic_str << endl;
            double lon, lat, alt, acc;
            long long tim; 
            long long rx = 0, tx = 0;
     
            if (sscanf(coords.c_str(), "%lf,%lf,%lf,%lf,%lld", &lon, &lat, &alt, &acc, &tim) == 5) {
            bool traffic_parsed = false;

            if (sscanf(traffic_str.c_str(), "%lld TX=%lld", &rx, &tx) == 2) {
                cout << "Парсинг: rx=" << rx << " tx=" << tx << endl;
                traffic_parsed = true;
            }

            if (traffic_parsed) {
                lock_guard<mutex> gui(*mtx);
                loc->longitude = lon;
                loc->latitude = lat;
                loc->altitude = alt;
                loc->accuracy = acc;                 
                loc->current_time = tim;
                loc->cell_info = cell_info;
                loc->traffic_rx = rx;                
                loc->traffic_tx = tx;               
                
                F << "Longitude: " << loc->longitude << ", Latitude: " << loc->latitude << ", Altitude: " << loc->altitude<< ", Accuracy: " << loc->accuracy << ", Time: " 
                << loc->current_time << ", Cell Info: " << loc->cell_info << ", Traffic RX: " << loc->traffic_rx << ", Traffic TX: " << loc->traffic_tx << endl;
                F.flush();
                } else {
            cout << "Ошибка парсинга трафика: " << traffic_str << endl;
        }
    } else {
        cout << "Ошибка парсинга координат: " << coords << endl;
    }
}
        
        zmq::message_t reply(3);
        memcpy(reply.data(), "OK", 3);
        socket.send(reply, zmq::send_flags::none);
        cout << "Кол-во полученных данных: " << count << endl;
    }
    
}
    catch(const zmq::error_t& e){
        cerr << "ZeroMQ ошибка: " << e.what() << endl;
    }
    catch(const exception& e) {
        cerr << "Ошибка сервера: " << e.what() << endl;
    }
    F.close();
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
void parseCellInfo(const string& cell) {

    if (cell.find("LTE:") != string::npos || cell.find("CellInfoLte") != string::npos) {

        ImGui::TextColored(ImVec4(0,1,0,1), "Network Type: LTE (4G)");

        ImGui::Text("  Cell Identity:");

        showField(cell, regex("mMcc=([0-9]+)"), "MCC");
        showField(cell, regex("mMnc=([0-9]+)"), "MNC");
        showField(cell, regex("mCi=([0-9]+)"), "Cell ID");
        showField(cell, regex("mPci=([0-9]+)"), "PCI");
        showField(cell, regex("mTac=([0-9]+)"), "TAC");
        showField(cell, regex("mEarfcn=([0-9]+)"), "EARFCN");
        showField(cell, regex("mBandwidth=([0-9]+)"), "Bandwidth");

        ImGui::Text("  Signal Strength:");

        showField(cell, regex("rsrp=(-?[0-9]+)"), "RSRP", "dBm");
        showField(cell, regex("rsrq=(-?[0-9]+)"), "RSRQ", "dB");
        showField(cell, regex("rssi=(-?[0-9]+)"), "RSSI", "dBm");
        showField(cell, regex("rssnr=([0-9]+)"), "RSSNR");
        showField(cell, regex("cqi=([0-9]+)"), "CQI");
        showField(cell, regex("ta=([0-9]+)"), "Timing Advance");
    }

    else if (cell.find("GSM:") != string::npos || cell.find("CellInfoGsm") != string::npos) {

        ImGui::TextColored(ImVec4(1,1,0,1), "Network Type: GSM (2G/3G)");

        ImGui::Text("  Cell Identity:");

        showField(cell, regex("mMcc=([0-9]+)"), "MCC");
        showField(cell, regex("mMnc=([0-9]+)"), "MNC");
        showField(cell, regex("mArfcn=([0-9]+)"), "ARFCN");
        showField(cell, regex("mBsic=([0-9]+)"), "BSIC");
        showField(cell, regex("mLac=([0-9]+)"), "LAC");
        showField(cell, regex("rssi=([0-9]+)"), "RSSI");
    }

    else if (cell.find("NR:") != string::npos || cell.find("CellInfoNr") != string::npos) {

        ImGui::TextColored(ImVec4(1,0,0,1), "Network Type: NR (5G)");

        showField(cell, regex("mNci=([0-9]+)"), "NCI");
        showField(cell, regex("mPci=([0-9]+)"), "PCI");
        showField(cell, regex("mTac=([0-9]+)"), "TAC");
        showField(cell, regex("mNrargcn=([0-9]+)"), "NRARFCN");

        showField(cell, regex("SS-RSRP=(-?[0-9]+)"), "SS-RSRP", "dBm");
        showField(cell, regex("SS-RSRQ=(-?[0-9]+)"), "SS-RSRQ", "dB");
        showField(cell, regex("SS-SINR=(-?[0-9]+)"), "SS-SINR", "dB");
    }
}
void run_gui(location *loc, mutex *mtx){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Location and Cell Info", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 900, 800, SDL_WINDOW_OPENGL);
    SDL_GLContext gl_context = SDL_GL_CreateContext(window);

    ImGui::CreateContext();
    ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
    ImGui_ImplOpenGL3_Init("#version 330");

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (event.type == SDL_QUIT) running = false;
        }

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
        
        ImGui::Begin("Location and Cell Info");
        lock_guard<mutex> lock(*mtx);
    
        ImGui::TextColored(ImVec4(1,1,0,1), "Location");
        ImGui::Text("Latitude:  %.6f", loc->latitude);
        ImGui::Text("Longitude: %.6f", loc->longitude);
        ImGui::Text("Altitude:  %.2f m", loc->altitude);
        ImGui::Text("Accuracy:  %.2f m", loc->accuracy); 
        ImGui::Text("Time:      %.0f ms", loc->current_time);
        
        ImGui::Separator();
  
        ImGui::TextColored(ImVec4(0,1,1,1), "CellInfo");
        string cell = loc->cell_info;
       
        parseCellInfo(cell);
      
        ImGui::Separator();
        
        ImGui::TextColored(ImVec4(1,0.5,0,1), "Network traffic");
 
        double rx_mb = loc->traffic_rx / (1024.0 * 1024.0);
        double tx_mb = loc->traffic_tx / (1024.0 * 1024.0);
        double total_mb = (loc->traffic_rx + loc->traffic_tx) / (1024.0 * 1024.0);
        
        ImGui::Text("Received (RX): %lld bytes (%.2f MB)", loc->traffic_rx, rx_mb);
        ImGui::Text("Sent (TX):     %lld bytes (%.2f MB)", loc->traffic_tx, tx_mb);
        ImGui::TextColored(ImVec4(0,1,0,1), "Total:          %.2f MB", total_mb);
        
        ImGui::End();

        ImGui::Render();
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();
    SDL_GL_DeleteContext(gl_context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

int main(int argc, char *argv[]) {
    static location locationInfo;
    static mutex mtx;
    thread zmq_thread(thread1, &locationInfo, &mtx);
    thread gui_thread(run_gui, &locationInfo, &mtx);
    zmq_thread.join();
    gui_thread.join();
    return 0;
}