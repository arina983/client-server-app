#include "utils.h"
#include "common.h"
#include <iostream>
#include <climits>

void showField(const std::string& text, const std::regex& r, const char* label, const char* unit) {
    std::smatch match;
    if (std::regex_search(text, match, r)) {
        if (unit && strlen(unit) > 0)
            ImGui::Text("    %s: %s %s", label, match[1].str().c_str(), unit);
        else
            ImGui::Text("    %s: %s", label, match[1].str().c_str());
    }
}

void printCellInfoPretty(const std::string& cell) {
    std::smatch m;
    std::cout << "CELL INFO\n";

    if (std::regex_search(cell, m, std::regex("mMcc=([0-9]+)")))      std::cout << "MCC: " << m[1] << std::endl;
    if (std::regex_search(cell, m, std::regex("mMnc=([0-9]+)")))      std::cout << "MNC: " << m[1] << std::endl;
    if (std::regex_search(cell, m, std::regex("mCi=([0-9]+)")))       std::cout << "Cell ID: " << m[1] << std::endl;
    if (std::regex_search(cell, m, std::regex("mPci=([0-9]+)")))      std::cout << "PCI: " << m[1] << std::endl;
    if (std::regex_search(cell, m, std::regex("mTac=([0-9]+)")))      std::cout << "TAC: " << m[1] << std::endl;
    if (std::regex_search(cell, m, std::regex("mEarfcn=([0-9]+)")))   std::cout << "EARFCN: " << m[1] << std::endl;
    if (std::regex_search(cell, m, std::regex("rsrp=(-?[0-9]+)")))    std::cout << "RSRP: " << m[1] << " dBm\n";
    if (std::regex_search(cell, m, std::regex("rsrq=(-?[0-9]+)")))    std::cout << "RSRQ: " << m[1] << " dB\n";
    if (std::regex_search(cell, m, std::regex("rssi=(-?[0-9]+)")))    std::cout << "RSSI: " << m[1] << " dBm\n";

    std::cout << "=================\n";
}

void parseCellInfo(const std::string& cell) {
    if (cell.find("LTE") != std::string::npos) {
        ImGui::TextColored(ImVec4(0,1,0,1), "LTE (4G)");
        showField(cell, std::regex("rsrp=(-?[0-9]+)"), "RSRP", "dBm");
        showField(cell, std::regex("rsrq=(-?[0-9]+)"), "RSRQ", "dB");
        showField(cell, std::regex("rssi=(-?[0-9]+)"), "RSSI", "dBm");
    }
}

void updateHistories(const json& j, double t, Location& new_loc) {
    if (!j.contains("cell") || !j["cell"].is_array()) return;

    for (auto& c : j["cell"]) {
        int pci = c.value("pci", -1);
        if (pci < 0) continue;

        int rsrp = c.value("rsrp", INT_MIN);
        int rsrq = c.value("rsrq", INT_MIN);
        int rssi = c.value("rssi", INT_MIN);
        int sinr = c.value("sinr", INT_MIN);

        time_histories[pci].push_back(t);
        if (rsrp != INT_MIN) rsrp_histories[pci].push_back(static_cast<double>(rsrp));
        if (rsrq != INT_MIN) rsrq_histories[pci].push_back(static_cast<double>(rsrq));
        if (rssi != INT_MIN) rssi_histories[pci].push_back(static_cast<double>(rssi));
        if (sinr != INT_MIN) sinr_histories[pci].push_back(static_cast<double>(sinr));

        if (new_loc.rsrp == 0 || c.value("isRegistered", false)) {
            new_loc.rsrp = rsrp;
            new_loc.rsrq = rsrq;
            new_loc.rssi = rssi;
            new_loc.sinr = sinr;
        }
    }
    trimHistories();
}

void trimHistories(size_t max_size) {
    for (auto& [pci, vec] : time_histories) {
        if (vec.size() > max_size) {
            vec.erase(vec.begin());
            if (!rsrp_histories[pci].empty()) rsrp_histories[pci].erase(rsrp_histories[pci].begin());
            if (!rsrq_histories[pci].empty()) rsrq_histories[pci].erase(rsrq_histories[pci].begin());
            if (!rssi_histories[pci].empty()) rssi_histories[pci].erase(rssi_histories[pci].begin());
            if (!sinr_histories[pci].empty()) sinr_histories[pci].erase(sinr_histories[pci].begin());
        }
    }
}