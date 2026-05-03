#include "gui.h"
#include "utils.h"
#include <SDL2/SDL.h>
#include <GL/glew.h>
#include "imgui.h"
#include "implot.h"
#include "backends/imgui_impl_sdl2.h"
#include "backends/imgui_impl_opengl3.h"
#include <string>
#include <mutex>
#include "TileManager.h"
#include "map.h"

static TileManager tileManager;

void run_gui(Location* loc, std::mutex* mtx) {
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

        Location local_copy;
        {
            std::lock_guard<std::mutex> lock(*mtx);
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
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "Signal");

        ImVec4 color = (local_copy.rsrp > -90) ? ImVec4(0,1,0,1) :
                       (local_copy.rsrp > -110) ? ImVec4(1,1,0,1) : ImVec4(1,0,0,1);

        ImGui::TextColored(color, "RSRP: %d dBm", local_copy.rsrp);
        ImGui::TextColored(color, "RSRQ: %d dB",  local_copy.rsrq);
        ImGui::TextColored(color, "RSSI: %d dBm", local_copy.rssi);

        ImGui::Separator();

        if (ImPlot::BeginPlot("RSRP", ImVec2(-1, 280))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
            ImPlot::SetupAxis(ImAxis_Y1, "RSRP (dBm)");
            std::lock_guard<std::mutex> lock(*mtx);
            for (auto& [pci, data] : rsrp_histories) {
                if (!data.empty())
                    ImPlot::PlotLine(("PCI " + std::to_string(pci)).c_str(), time_histories[pci].data(), data.data(), (int)data.size());
            }
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("RSRQ", ImVec2(-1, 220))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
            ImPlot::SetupAxis(ImAxis_Y1, "RSRQ (dB)");
            std::lock_guard<std::mutex> lock(*mtx);
            for (auto& [pci, data] : rsrq_histories) {
                if (!data.empty())
                    ImPlot::PlotLine(("PCI " + std::to_string(pci)).c_str(), time_histories[pci].data(), data.data(), (int)data.size());
            }
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("RSSI", ImVec2(-1, 220))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
            ImPlot::SetupAxis(ImAxis_Y1, "RSSI (dBm)");
            std::lock_guard<std::mutex> lock(*mtx);
            for (auto& [pci, data] : rssi_histories) {
                if (!data.empty())
                    ImPlot::PlotLine(("PCI " + std::to_string(pci)).c_str(), time_histories[pci].data(), data.data(), (int)data.size());
            }
            ImPlot::EndPlot();
        }

        if (ImPlot::BeginPlot("SINR", ImVec2(-1, 220))) {
            ImPlot::SetupAxis(ImAxis_X1, "Time (s)");
            ImPlot::SetupAxis(ImAxis_Y1, "SINR (dB)");
            std::lock_guard<std::mutex> lock(*mtx);
            for (auto& [pci, data] : sinr_histories) {
                if (!data.empty()) {
                    std::string label = "PCI " +
                    std::to_string(pci);
                    ImPlot::PlotLine(label.c_str(), time_histories[pci].data(), data.data(), (int)data.size());
                }
            }
            ImPlot::EndPlot();
        }
        ImGui::End();
        ImGui::Begin("Map");

        static int zoom = 15;
        if (ImGui::IsWindowHovered() && ImGui::GetIO().MouseWheel != 0) {
            zoom += (int)ImGui::GetIO().MouseWheel;
            if (zoom < 0) zoom = 0;
            if (zoom > 19) zoom = 19;
        }

        ImGui::Text("Zoom: %d", zoom);

        TileCoordsFloat playerPos = OsmUtils::geoToTileFloat(local_copy.latitude, local_copy.longitude, zoom);
        ImVec2 plot_size = ImGui::GetContentRegionAvail();

        double view_width = plot_size.x / 256.0;
        double view_height = plot_size.y / 256.0;

        ImPlot::SetNextAxesLimits(playerPos.x - view_width/2, playerPos.x + view_width/2, -playerPos.y - view_height/2, -playerPos.y + view_height/2, ImGuiCond_Always);

        if (ImPlot::BeginPlot("##OSM", ImVec2(-1, -1), ImPlotFlags_Equal | ImPlotFlags_NoMenus | ImPlotFlags_NoBoxSelect)) {
            
            int start_x = (int)floor(playerPos.x - view_width/2) - 1;
            int end_x   = (int)ceil(playerPos.x + view_width/2) + 1;
            int start_y = (int)floor(playerPos.y - view_height/2) - 1;
            int end_y   = (int)ceil(playerPos.y + view_height/2) + 1;

            for (int x = start_x; x <= end_x; x++) {
                for (int y = start_y; y <= end_y; y++) {
                    TileCoords t = {x, y, zoom};
                    GLuint id = tileManager.getTileTexture(t);
                    if (id != 0) {
                        ImPlot::PlotImage("##tile", (void*)(intptr_t)id, ImVec2(x, -(y + 1)), ImVec2(x + 1, -y));
                    }
                }
            }

            double pX = playerPos.x; double pY = -playerPos.y;
            ImPlot::SetNextMarkerStyle(ImPlotMarker_Circle, 8, ImVec4(1,0,0,1), 1, ImVec4(1,1,1,1));
            ImPlot::PlotScatter("I", &pX, &pY, 1);

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