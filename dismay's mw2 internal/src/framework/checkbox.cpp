#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "checkbox.hpp"
#include "draw_helpers.hpp"
#include "settings.h"

#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"

#include <map>

namespace {

struct CheckboxAnim
{
	ImVec4 bgColor;
	ImVec4 circleColor;
	ImVec4 textColor;
	float circleInterp;
};

}

bool ksd::Checkbox(const char* label, bool* value) noexcept
{
	if (!label || !value)
	{
		return false;
	}

	ImGuiWindow* const window = ImGui::GetCurrentWindow();

	if (!window || window->SkipItems)
	{
		return false;
	}

	ImGuiContext& g = *GImGui;
	const ImGuiStyle& style = g.Style;

	const ImGuiID id = window->GetID(label);
	const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr, true);

	const float trackWidth = ImMax(ImGui::GetWindowWidth() - 46.f, 40.f);
	const float knobRef = 12.f;
	const ImVec2 origin(window->DC.CursorPos);

	const float rowHeight =
		ImMax(ImMax(labelSize.y + style.FramePadding.y, knobRef + 10.f), 24.f);

	const ImRect hitRect(origin, origin + ImVec2(knobRef + trackWidth, rowHeight));

	ImGui::ItemSize(hitRect);

	if (!ImGui::ItemAdd(hitRect, id))
	{
		return false;
	}

	bool hovered = false;
	bool held = false;
	const bool pressed = ImGui::ButtonBehavior(hitRect, id, &hovered, &held);

	if (ImGui::IsItemClicked())
	{
		*value = !(*value);
		ImGui::MarkItemEdited(id);
	}

	static std::map<ImGuiID, CheckboxAnim> animCache;
	auto it = animCache.find(id);

	if (it == animCache.end())
	{
		CheckboxAnim initial{};
		initial.bgColor = colors::checkbox::checkbox_bg_inactive;
		initial.circleColor = colors::checkbox::circle_inactive;
		initial.textColor = colors::checkbox::text_inactive;
		initial.circleInterp = 0.f;
		animCache.emplace(id, initial);
		it = animCache.find(id);
	}

	CheckboxAnim* anim = &it->second;
	const float dt = g.IO.DeltaTime;

	anim->circleColor = ImLerp(
		anim->circleColor,
		(*value) ? colors::accent_color : colors::checkbox::circle_inactive,
		dt * 6.f);

	anim->bgColor = ImLerp(
		anim->bgColor,
		(*value) ? colors::checkbox::checkbox_bg_active : colors::checkbox::checkbox_bg_inactive,
		dt * 6.f);

	const ImVec4 targetText = (*value)
		? colors::checkbox::text_active
		: (hovered ? colors::checkbox::text_hovered : colors::checkbox::text_inactive);

	anim->textColor = ImLerp(anim->textColor, targetText, dt * 6.f);

	anim->circleInterp = ImLerp(
		anim->circleInterp,
		(*value) ? -13.f : 0.f,
		dt * 9.f);

	const ImRect knobBB(origin, origin + ImVec2(knobRef, knobRef));
	ImDrawList* const dl = ImGui::GetWindowDrawList();

	const ImVec2 trayMin = knobBB.Min + ImVec2((trackWidth - 20.f), 0.f);
	const ImVec2 trayMax = knobBB.Max + ImVec2(trackWidth + 4.f, 7.5f);

	dl->AddRectFilled(trayMin, trayMax, ImGui::GetColorU32(anim->bgColor), settings::checkbox_rounding);

	const ImVec2 circleCenter = knobBB.Min + ImVec2((trackWidth - 8.5f - anim->circleInterp), 9.5f);
	const float circleRadius = 6.f;
	const ImU32 circlePacked = ImGui::GetColorU32(anim->circleColor);

	dl->AddCircleFilled(circleCenter, circleRadius, circlePacked);

	ksd::DrawCircleShadow(dl, circleCenter, circleRadius, circlePacked, 6.5f, ImVec2(0.f, 0.f));

	if (fonts::inter_bold_font2)
	{
		const ImVec2 captionPos = ImVec2(knobBB.Max.x - knobRef - 9.f, knobBB.Min.y + 3.5f);

		dl->AddText(
			fonts::inter_bold_font2,
			17.f,
			captionPos,
			ImGui::GetColorU32(anim->textColor),
			label,
			ImGui::FindRenderedTextEnd(label));
	}

	ImGui::Dummy(ImVec2(1.f, 2.f));

	return pressed;
}
