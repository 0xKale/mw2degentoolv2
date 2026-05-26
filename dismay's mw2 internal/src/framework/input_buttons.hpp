#pragma once

#include "../../ext/imgui/imgui.h"

namespace ksd {

bool InputFloat(const char* label, float* value) noexcept;
bool InputInt(const char* label, int* value) noexcept;


bool InputTextOnly(const char* id, char* buf, size_t buf_size, float width, ImGuiInputTextFlags flags = 0) noexcept;

}
