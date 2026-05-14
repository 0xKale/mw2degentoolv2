#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "tab.hpp"
#include "settings.h"

#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"

#include <map>

namespace {

struct TabAnim
{
	ImVec4 textColor;
	float accentAlpha;
};

}

namespace ImGui {

bool Tab(const char* label, const char* icon, const ImVec2& sizeArg, bool active)
{
	ImGuiWindow* window = GetCurrentWindow();

	if (window->SkipItems)
	{
		return false;
	}

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;
	const ImGuiID id = window->GetID(label);
	const ImVec2 labelSize = CalcTextSize(label, NULL, true);
	const ImVec2 pos = window->DC.CursorPos;
	ImVec2 size = CalcItemSize(sizeArg, labelSize.x + style.FramePadding.x * 2.0f, labelSize.y + style.FramePadding.y * 2.0f);
	const ImRect rect(pos, pos + size);

	ItemSize(rect, style.FramePadding.y);

	if (!ItemAdd(rect, id))
	{
		return false;
	}

	bool hovered = false;
	bool held = false;
	const bool pressed = ButtonBehavior(rect, id, &hovered, &held);

	if (pressed)
	{
		MarkItemEdited(id);
	}

	static std::map<ImGuiID, TabAnim> animCache;
	auto it = animCache.find(id);

	if (it == animCache.end())
	{
		animCache.emplace(id, TabAnim());
		it = animCache.find(id);
	}

	const ImVec4 targetText = active ? colors::tabs::text_active
		: hovered ? colors::tabs::text_hovered
		: colors::tabs::text_inactive;

	it->second.textColor = ImLerp(it->second.textColor, targetText, g.IO.DeltaTime * 6.f);

	const float targetAlpha = active ? 1.f : hovered ? 0.7f : 0.3f;
	it->second.accentAlpha = ImLerp(it->second.accentAlpha, targetAlpha, g.IO.DeltaTime * 6.f);

	if (icon != nullptr && icon[0] != '\0')
	{
		ImFont* iconFont = fonts::fa_font;

		if (!iconFont)
		{
			iconFont = GetFont();
		}

		ImVec4 iconTint = colors::accent_color;
		iconTint.w *= it->second.accentAlpha;

		const ImU32 iconPacked = GetColorU32(iconTint);
		const char* const glyphEnd = FindRenderedTextEnd(icon);

		window->DrawList->AddText(
			iconFont, 17.f, rect.Min + ImVec2(14.f, 10.f),
			iconPacked, icon, glyphEnd, 0.f, nullptr);
	}

	ImFont* labelFont = fonts::inter_font;

	if (!labelFont)
	{
		labelFont = GetFont();
	}

	window->DrawList->AddText(
		labelFont, 17.f, rect.Min + ImVec2(37.f, 10.f),
		ImColor(it->second.textColor), label);

	IMGUI_TEST_ENGINE_ITEM_INFO(id, label, g.LastItemData.StatusFlags);

	return pressed;
}

}
