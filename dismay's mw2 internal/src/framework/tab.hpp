#pragma once

#include "../../ext/imgui/imgui.h"

namespace ImGui {

IMGUI_API bool Tab(
	const char* label,
	const char* icon,
	const ImVec2& size,
	bool active);

}
