#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "slider.hpp"
#include "draw_helpers.hpp"
#include "settings.h"

#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"

#include <map>

namespace {

struct SliderAnim
{
	ImVec4 textColor{};
	float accentWidth{ -2.f };
};

static bool SliderScalar(
	const char* label,
	ImGuiDataType dataType,
	void* payload,
	const void* pMin,
	const void* pMax,
	const char* format,
	ImGuiSliderFlags flags) noexcept
{
	if (!label || !payload || !pMin || !pMax)
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

	constexpr float trackSpan = 140.f;
	constexpr float trackHeight = 6.f;
	constexpr float labelGap = 6.f;
	constexpr float trackRounding = 5.f;
	const float contentWidth = ImGui::GetWindowWidth() - 35.f;
	const float trackLeadX = ImMax(contentWidth - trackSpan, 40.f);

	const ImVec2 origin(window->DC.CursorPos);

	ImVec2 labelSize;
	if (fonts::inter_bold_font2)
	{
		ImGui::PushFont(fonts::inter_bold_font2);
		labelSize = ImGui::CalcTextSize(label, nullptr, true);
		ImGui::PopFont();
	}
	else
	{
		labelSize = ImGui::CalcTextSize(label, nullptr, true);
	}

	const ImRect trackRect(
		origin + ImVec2(trackLeadX, labelSize.y + labelGap),
		origin + ImVec2(contentWidth, labelSize.y + labelGap + trackHeight));

	const ImRect totalRect(
		origin,
		ImVec2(origin.x + contentWidth, trackRect.Max.y));

	ImGui::ItemSize(totalRect);

	const bool allowInput = (flags & ImGuiSliderFlags_NoInput) == 0;

	const bool visible = ImGui::ItemAdd(
		totalRect, id, &trackRect,
		allowInput ? ImGuiItemFlags_Inputable : 0);

	if (!visible)
	{
		return false;
	}

	const char* resolvedFormat = format ? format : ImGui::DataTypeGetInfo(dataType)->PrintFmt;

	const bool isHovered = ImGui::ItemHoverable(trackRect, id, g.LastItemData.ItemFlags);

	bool tempInput = allowInput && ImGui::TempInputIsActive(id);

	if (!tempInput)
	{
		const bool clicked = isHovered && ImGui::IsMouseClicked(0, ImGuiInputFlags_None, id);
		const bool shouldActivate = clicked || g.NavActivateId == id;

		if (shouldActivate && clicked)
		{
			ImGui::SetKeyOwner(ImGuiKey_MouseLeft, id);
		}

		if (shouldActivate && allowInput)
		{
			if ((clicked && g.IO.KeyCtrl) ||
				(g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput) != 0))
			{
				tempInput = true;
			}
		}

		if (shouldActivate)
		{
			memcpy(&g.ActiveIdValueOnActivation, payload,
				static_cast<size_t>(ImGui::DataTypeGetInfo(dataType)->Size));
		}

		if (shouldActivate && !tempInput)
		{
			ImGui::SetActiveID(id, window);
			ImGui::SetFocusID(id, window);
			ImGui::FocusWindow(window);
			g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
		}
	}

	if (tempInput)
	{
		const bool clamp = (flags & ImGuiSliderFlags_ClampOnInput) != 0;
		return ImGui::TempInputScalar(
			trackRect, id, label, dataType, payload, resolvedFormat,
			clamp ? pMin : nullptr, clamp ? pMax : nullptr);
	}

	ImRect grabRect{};
	const bool changed = ImGui::SliderBehavior(
		trackRect, id, dataType, payload, pMin, pMax,
		resolvedFormat, flags, &grabRect);

	if (changed)
	{
		ImGui::MarkItemEdited(id);
	}

	static std::map<ImGuiID, SliderAnim> animCache;
	auto it = animCache.find(id);

	if (it == animCache.end())
	{
		SliderAnim initial{};
		initial.textColor = colors::slider::text_inactive;
		animCache.emplace(id, initial);
		it = animCache.find(id);
	}

	SliderAnim* anim = &it->second;

	char valueBuf[64]{};
	const char* valueEnd = valueBuf + ImGui::DataTypeFormatString(
		valueBuf, (int)IM_ARRAYSIZE(valueBuf), dataType, payload, resolvedFormat);

	const bool hasGrab = grabRect.Max.x > grabRect.Min.x;

	constexpr float accentPastGrab = 0.01f;
	const float grabWidth = hasGrab ? (grabRect.Max.x - grabRect.Min.x) : 0.f;
	const float accentTrail = hasGrab
		? ImMin(grabRect.Max.x + grabWidth * accentPastGrab, trackRect.Max.x)
		: trackRect.Min.x;

	const float targetAccent = ImClamp(accentTrail - trackRect.Min.x, 0.f, trackRect.GetWidth());

	if (anim->accentWidth < 0.f)
	{
		anim->accentWidth = targetAccent;
	}

	if (g.ActiveId == id)
	{
		anim->accentWidth = targetAccent;
	}
	else
	{
		anim->accentWidth = ImLerp(anim->accentWidth, targetAccent, 0.35f);
	}

	const float clampedAccent = ImClamp(anim->accentWidth, 0.f, trackRect.GetWidth());

	const ImVec4 targetText = g.ActiveId == id
		? colors::slider::text_active
		: (isHovered ? colors::slider::text_hovered : colors::slider::text_inactive);

	const float interpSpeed = g.IO.DeltaTime * 6.f;
	anim->textColor = ImLerp(anim->textColor, targetText, interpSpeed);

	ImDrawList* const dl = window->DrawList;

	dl->AddRectFilled(trackRect.Min, trackRect.Max,
		ImGui::GetColorU32(colors::slider::slider_inactive), trackRounding);

	const ImVec2 accentMax(trackRect.Min.x + clampedAccent, trackRect.Max.y);

	const ImU32 accentPacked = ksd::ColorWithAlpha(colors::accent_color, style.Alpha);
	const ImU32 accentGlow = ksd::ColorWithAlpha(colors::accent_color, style.Alpha * 0.48f);

	if (clampedAccent > 0.5f)
	{
		const float glowPad = 10.f;
		const ImRect glowClip(
			ImVec2(trackRect.Min.x - glowPad, trackRect.Min.y - glowPad),
			ImVec2(accentMax.x + glowPad, trackRect.Max.y + glowPad));

		ImGui::PushClipRect(glowClip.Min, glowClip.Max, false);

		dl->AddRectFilled(trackRect.Min, accentMax, accentPacked, trackRounding);

		ksd::DrawRectShadow(dl, trackRect.Min, accentMax, accentGlow,
			5.5f, ImVec2(0.f, 0.f), ImDrawFlags_None, trackRounding);

		ImGui::PopClipRect();
	}

	if (fonts::inter_bold_font2)
	{
		const ImVec2 captionPos(origin.x - 9.f, origin.y + 3.5f);

		dl->AddText(fonts::inter_bold_font2, 17.f, captionPos,
			ImGui::GetColorU32(anim->textColor),
			label, ImGui::FindRenderedTextEnd(label));
	}

	if (g.ActiveId != id)
	{
		ImGui::PushStyleColor(ImGuiCol_Text, anim->textColor);
		if (fonts::inter_bold_font2)
		{
			ImGui::PushFont(fonts::inter_bold_font2);
		}
		ImGui::RenderTextClipped(
			totalRect.Min, totalRect.Max, valueBuf, valueEnd,
			nullptr, ImVec2(1.f, 0.f));
		if (fonts::inter_bold_font2)
		{
			ImGui::PopFont();
		}
		ImGui::PopStyleColor();
	}

	if (g.ActiveId == id)
	{
		const ImVec2 tipSize = ImGui::CalcTextSize(valueBuf, valueEnd, true);
		const float tipCenterX = grabRect.GetCenter().x;

		const ImVec2 chipMin(
			tipCenterX - tipSize.x * 0.5f - 5.f,
			trackRect.Min.y - tipSize.y - 10.f);
		const ImVec2 chipMax(
			tipCenterX + tipSize.x * 0.5f + 4.f,
			trackRect.Min.y - 5.f);

		ImGui::GetForegroundDrawList()->AddRectFilled(
			chipMin, chipMax, ImColor(25, 25, 26), trackRounding);

		ImGui::GetForegroundDrawList()->AddText(
			ImVec2(
				chipMin.x + (chipMax.x - chipMin.x - tipSize.x) * 0.5f,
				chipMin.y + (chipMax.y - chipMin.y - tipSize.y) * 0.5f),
			ImGui::GetColorU32(anim->textColor),
			valueBuf, valueEnd);
	}

	ImGui::Dummy(ImVec2(0.f, 3.f));

	return changed;
}

}

bool ksd::SliderInt(
	const char* label,
	int* value,
	const int min,
	const int max,
	const char* format,
	const ImGuiSliderFlags flags) noexcept
{
	char labelBuf[256];
	ImFormatString(labelBuf, IM_ARRAYSIZE(labelBuf), "%s##%p", label, static_cast<const void*>(value));
	const char* resolved = format ? format : "%d";
	return SliderScalar(labelBuf, ImGuiDataType_S32, value, &min, &max, resolved, flags);
}

bool ksd::SliderFloat(
	const char* label,
	float* value,
	const float min,
	const float max,
	const char* format,
	const ImGuiSliderFlags flags) noexcept
{
	char labelBuf[256];
	ImFormatString(labelBuf, IM_ARRAYSIZE(labelBuf), "%s##%p", label, static_cast<const void*>(value));
	const char* resolved = format ? format : "%.3f";
	return SliderScalar(labelBuf, ImGuiDataType_Float, value, &min, &max, resolved, flags);
}
