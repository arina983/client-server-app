#pragma once

#include <string>
#include <regex>
#include "imgui.h"
#include "common.h"

void updateHistories(const json& j, double t, Location& new_loc);
void trimHistories(size_t max_size = 3000);