#pragma once

#include "../../ext/imgui/imgui.h"

namespace ksd {

bool ButtonEx(const char* label, const ImVec2& size, ImGuiButtonFlags flags = 0) noexcept;
bool Button(const char* label, const ImVec2& size = ImVec2(0, 0)) noexcept;

}

