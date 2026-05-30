#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "button.hpp"
#include "draw_helpers.hpp"
#include "settings.h"

#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"

#include <map>

namespace {


struct ButtonState
{
	ImVec4 outline{};
	ImVec4 text{};
	float rippleRadius = 0.f;
	bool rippleActive = false;
	float rippleAlpha = 0.f;
};

}

bool ksd::ButtonEx(const char* label, const ImVec2& sizeArg, const ImGuiButtonFlags flags, const bool selected) noexcept
{
	ImGuiWindow* const window = ImGui::GetCurrentWindow();
	if (!window || window->SkipItems)
	{
		return false;
	}

	ImGuiContext& g = *GImGui;
	const ImGuiID id = window->GetID(label);
	const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);
	const ImVec2 pos = window->DC.CursorPos;

	ImVec2 size = ImGui::CalcItemSize(sizeArg, labelSize.x, labelSize.y);

	if (size.x <= 0.f)
	{
		size.x = sizeArg.x;
	}
	if (size.y <= 0.f)
	{
		size.y = 30.f;
	}

	const ImRect bb(pos - ImVec2(10.f, 0.f), pos + size);

	ImGui::ItemSize(ImRect(bb.Min, bb.Max + ImVec2(0.f, 2.f)));
	if (!ImGui::ItemAdd(bb, id))
	{
		return false;
	}

	bool hovered = false;
	bool held = false;
	const bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);
	(void)held;

	static std::map<ImGuiID, ButtonState> anim;
	auto it = anim.find(id);
	if (it == anim.end())
	{
		anim.emplace(id, ButtonState{});
		it = anim.find(id);
	}

	ButtonState& st = it->second;

	if (pressed && !st.rippleActive && st.rippleAlpha <= 0.1f)
	{
		st.rippleActive = true;
	}

	st.rippleAlpha = ImClamp(
		st.rippleAlpha + (4.f * g.IO.DeltaTime * (st.rippleActive ? 1.f : -1.f)),
		0.f, 1.f);

	if (st.rippleActive)
	{
		st.rippleRadius += (size.x / 2.f) * g.IO.DeltaTime * 5.f;
	}

	if (st.rippleAlpha <= 0.1f)
	{
		st.rippleRadius = 0.f;
	}

	if (st.rippleRadius >= size.x / 2.f)
	{
		st.rippleActive = false;
	}

	const ImVec4 idleOutline =
		selected ? colors::button::button_outline_selected : colors::button::button_bg;
	st.outline = ImLerp(
		st.outline,
		st.rippleActive ? colors::accent_color : idleOutline,
		g.IO.DeltaTime * 6.f);

	const ImVec4 targetTextColor =
		st.rippleActive || hovered || selected ? colors::button::text_active : colors::button::text_inactive;
	st.text = ImLerp(st.text, targetTextColor, g.IO.DeltaTime * 6.f);

	ImDrawList* const dl = ImGui::GetWindowDrawList();

	constexpr float buttonCornerRounding = 4.f;

	const ImVec4 fillColor =
		selected ? colors::button::button_bg_selected : colors::button::button_bg;
	dl->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(fillColor), buttonCornerRounding);

	{
		ImVec4 flash = colors::accent_color;
		flash.w *= 0.1f * st.rippleAlpha;
		dl->AddRectFilled(bb.Min, bb.Max, ImGui::GetColorU32(flash), buttonCornerRounding);
	}

	dl->AddRect(bb.Min, bb.Max, ImGui::GetColorU32(st.outline), buttonCornerRounding);

	const ImU32 outlinePacked = ImGui::GetColorU32(st.outline);
	ksd::DrawRectShadow(
		dl,
		bb.Min - ImVec2(1.f, 0.f),
		bb.Max + ImVec2(1.f, 0.f),
		outlinePacked,
		5.f,
		ImVec2(0.f, 0.f),
		ImDrawFlags_None,
		buttonCornerRounding);

	const ImVec2 textSize = ImGui::CalcTextSize(label);
	const float textX = bb.Min.x + (sizeArg.x - textSize.x) * 0.5f + 8.f;
	const float textY = bb.Max.y - textSize.y - (size.y - textSize.y) * 0.5f;

	dl->AddText(ImVec2(textX, textY), ImGui::GetColorU32(st.text), label);

	

	return pressed;
}

bool ksd::Button(const char* label, const ImVec2& size) noexcept
{
	return ButtonEx(label, size, ImGuiButtonFlags_None, false);
}

bool ksd::Button(const char* label, const ImVec2& size, const bool selected) noexcept
{
	return ButtonEx(label, size, ImGuiButtonFlags_None, selected);
}
