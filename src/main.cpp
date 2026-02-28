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

using namespace std;

struct location {
    double latitude;
    double longitude;
    double altitude;
    double current_time;
    string cell_info;
    
    location() : latitude(0), longitude(0), altitude(0), current_time(0), cell_info("") {}
};

void thread1(location *loc, mutex *mtx){
    ofstream F("visual.json");
    if (!F){
        cout << "Ошибка открытия файла";
        return;
    }
    try{
    zmq::context_t context(1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind("tcp://0.0.0.0:5556");
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
        cout << "!!! ПОЛУЧЕНО: " << data << endl;  
        
        count++;
 
        size_t delimiter_pos = data.find(" | ");
        if (delimiter_pos != string::npos) {
            string coords = data.substr(0, delimiter_pos);
            string cell_info = data.substr(delimiter_pos + 3);
            
            cout << "Координаты: " << coords << endl;
            
            double lon, lat, alt;
            long long tim; 
            
            if (sscanf(coords.c_str(), "%lf,%lf,%lf,%lld", &lon, &lat, &alt, &tim) == 4) {
                lock_guard<mutex> gui(*mtx);
                loc->longitude = lon;
                loc->latitude = lat;
                loc->altitude = alt;
                loc->current_time = tim;
                loc->cell_info = cell_info;
                
                cout << "! lon=" << lon << ", lat=" << lat 
                     << ", alt=" << alt << ", time=" << tim << endl;
                
                F << "Longitude: " << loc->longitude 
                  << ", Latitude: " << loc->latitude 
                  << ", Altitude: " << loc->altitude 
                  << ", Time: " << loc->current_time 
                  << ", Cell Info: " << loc->cell_info  << endl;
                F.flush();
            } else {
                cout << "ОШИБКА парсинга координат!" << endl;
            }
        } else {
            cout << "Нет разделителя ' | ' в данных!" << endl;
        }
        
        zmq::message_t reply(3);
        memcpy(reply.data(), "OK", 3);
        socket.send(reply, zmq::send_flags::none);

        cout << "кол-во полученных данных: " << count << endl;
    }
    
}
    catch(const zmq::error_t& e){
        cerr << "ZeroMQ ошибка: " << e.what() << endl;
    }
    catch(const exception& e) {
        cerr << "ошибка сервера: " << e.what() << endl;
    }
    F.close();
}

void run_gui(location *loc, mutex *mtx){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Location and Cell Info", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 700, SDL_WINDOW_OPENGL);
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
        ImGui::Text("Latitude: %.6f", loc->latitude);
        ImGui::Text("Longitude: %.6f", loc->longitude);
        ImGui::Text("Altitude: %.2f", loc->altitude);
        ImGui::Text("Time: %.0f", loc->current_time);
        ImGui::Separator();
        ImGui::Text("information about the cell");
        string cell = loc->cell_info;
        //тут переделать
        if (cell.find("LTE:") != string::npos || cell.find("CellInfoLte") != string::npos) {
            ImGui::TextColored(ImVec4(0,1,0,1), "network type: LTE (4G)");
        } else if (cell.find("GSM:") != string::npos || cell.find("CellInfoGsm") != string::npos) {
            ImGui::TextColored(ImVec4(1,1,0,1), "network type: GSM (2G)");
        } else if (cell.find("NR:") != string::npos || cell.find("CellInfoNr") != string::npos) {
            ImGui::TextColored(ImVec4(1,0,0,1), "network type: NR (5G)");
        }
        ImGui::TextWrapped("%s", cell.c_str());
        
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