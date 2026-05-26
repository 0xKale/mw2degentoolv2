#pragma once

#include "../../ext/imgui/imgui.h"

namespace ksd {

bool BeginChild(const char* icon, const char* id, float contentHeight, float width = 0.f);
void EndChild();

}
