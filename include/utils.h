#pragma once

#include <string>
#include <regex>
#include "imgui.h"
#include "common.h"

void showField(const std::string& text, const std::regex& r, const char* label, const char* unit = "");
void printCellInfoPretty(const std::string& cell);
void parseCellInfo(const std::string& cell);

void updateHistories(const json& j, double t, Location& new_loc);
void trimHistories(size_t max_size = 3000);