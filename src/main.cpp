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

using namespace std;

struct location {
    double latitude;
    double longitude;
    double altitude;
    double current_time;
    
    location() : latitude(0), longitude(0), altitude(0), current_time(0) {}
};

void thread1(location *loc){
    ofstream F("visual.json");
    if (!F){
        cout << "Ошибка открытия файла";
        return;
    }
    try{
    zmq::context_t context(1);
    zmq::socket_t socket (context, ZMQ_REP);
    socket.bind("tcp://*:5552");
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
        cout <<"данные получены: "<< data << endl;
        count++;
        // ТУТ ДОЛЖЕН БЫТЬ ПАРСИНГ 
        F << data << endl;
        zmq::message_t reply(3);
        memcpy(reply.data(), "OK", 3);
        socket.send(reply, zmq::send_flags::none);

        cout <<"кол-во полученных данных: " << count <<endl;
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

void run_gui(location *loc){
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Location", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 300, 400, SDL_WINDOW_OPENGL);
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
        
        ImGui::Begin("Coordinates");
        ImGui::Text("Latitude: %.6f", loc->latitude);
        ImGui::Text("Longitude: %.6f", loc->longitude);
        ImGui::Text("Altitude: %.2f", loc->altitude);
        ImGui::Text("Time: %.0f", loc->current_time);
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
    thread zmq_thread(thread1, &locationInfo);
    thread gui_thread(run_gui, &locationInfo);
    zmq_thread.join();
    gui_thread.join();
    return 0;
}