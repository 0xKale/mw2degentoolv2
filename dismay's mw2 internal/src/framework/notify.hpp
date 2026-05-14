#pragma once

#include <string_view>

#include "../../ext/imgui/imgui.h"

namespace notify {

void addNotify(std::string_view text, float time);
void setupNotify();

}

void SendNotify(const char* text, float time);
