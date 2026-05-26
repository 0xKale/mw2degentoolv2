#pragma once

#include "../../ext/imgui/imgui.h"

namespace ksd {

bool SliderInt(
	const char* label,
	int* value,
	int min,
	int max,
	const char* format = "%d",
	ImGuiSliderFlags flags = 0) noexcept;

bool SliderFloat(
	const char* label,
	float* value,
	float min,
	float max,
	const char* format = "%.3f",
	ImGuiSliderFlags flags = 0) noexcept;

}
