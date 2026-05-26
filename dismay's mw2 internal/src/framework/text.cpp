#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "text.hpp"
#include "settings.h"

#include "../../ext/imgui/imgui.h"

#include <cstdarg>

namespace {

constexpr float kTextOffsetX = -9.f;

}

void ksd::TextV(const char* fmt, va_list args)
{
	if (!fmt)
	{
		return;
	}

	const ImVec2 pos(ImGui::GetCursorPos());
	ImGui::SetCursorPos(ImVec2(pos.x + kTextOffsetX, pos.y));

	ImFont* const font = fonts::inter_bold_font2;
	if (font)
	{
		ImGui::PushFont(font);
	}

	ImGui::TextV(fmt, args);

	if (font)
	{
		ImGui::PopFont();
	}
}

void ksd::Text(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	TextV(fmt, args);
	va_end(args);
}

bool ksd::TextLinkOpenURL(const char* const label, const char* const url)
{
	if (!label)
	{
		return false;
	}

	const ImVec2 pos(ImGui::GetCursorPos());
	ImGui::SetCursorPos(ImVec2(pos.x + kTextOffsetX, pos.y));

	ImFont* const font = fonts::inter_bold_font2;
	if (font)
	{
		ImGui::PushFont(font);
	}

	const bool pressed = ImGui::TextLinkOpenURL(label, url);

	if (font)
	{
		ImGui::PopFont();
	}

	return pressed;
}
