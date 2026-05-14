#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "input_buttons.hpp"
#include "settings.h"

#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"

#include <algorithm>
#include <utility>

namespace {

template<typename T, typename F>
bool InputRowImpl(const char* label, T* value, const char* valueImgUiId, F&& drawField)
{
	if (!label || !value)
	{
		return false;
	}

	ImGui::PushID(label);

	const ImGuiStyle& style = ImGui::GetStyle();
	const float rowHeight = 30.f;

	ImFont* const boldFont = fonts::inter_bold_font2;

	const float framePadY = std::max(4.f, (rowHeight - ImGui::GetFontSize()) * 0.5f);

	if (boldFont)
	{
		ImGui::PushFont(boldFont);
	}

	const ImVec2 labelSize = ImGui::CalcTextSize(label, nullptr);

	if (boldFont)
	{
		ImGui::PopFont();
	}

	ImGui::AlignTextToFramePadding();

	const ImVec2 startPos(ImGui::GetCursorPos());
	constexpr float fullRowControlWidth = 280.f;
	const float rowRight = startPos.x + fullRowControlWidth;

	constexpr float labelOffsetX = -9.f;
	const ImVec2 labelDrawPos(startPos.x + labelOffsetX, startPos.y);
	ImGui::SetCursorPos(labelDrawPos);

	if (boldFont)
	{
		ImGui::PushFont(boldFont);
	}

	ImGui::PushStyleColor(ImGuiCol_Text, colors::checkbox::text_active);
	ImGui::TextUnformatted(label);
	ImGui::PopStyleColor();

	if (boldFont)
	{
		ImGui::PopFont();
	}

	const float labelEnd = labelDrawPos.x + labelSize.x;
	const float minGap = style.ItemInnerSpacing.x > 12.f ? style.ItemInnerSpacing.x : 12.f;

	const float minFieldStart = labelEnd + minGap;
	const float fieldEnd = rowRight;
	const float maxSpan = fieldEnd - minFieldStart;
	const float leftClamp = ImGui::GetWindowContentRegionMin().x;
	constexpr float kInputFieldMaxWidth = 85.f;
	const float drawWidth = std::min(kInputFieldMaxWidth, std::min(maxSpan, fieldEnd - leftClamp));

	if (drawWidth < 36.f)
	{
		ImGui::PopID();
		return false;
	}

	const float fieldStart = fieldEnd - drawWidth;
	ImGui::SetCursorPos(ImVec2(fieldStart, startPos.y));

	ImGuiContext& g = *GImGui;
	const ImGuiID valueId = ImGui::GetID(valueImgUiId);
	ImGuiInputTextState* const textState = ImGui::GetInputTextState(valueId);
	const bool useBlackText =
		g.ActiveId == valueId &&
		textState != nullptr &&
		textState->HasSelection();

	ImVec4 selectionBg = colors::accent_color;
	selectionBg.w *= 0.35f;

	const ImVec4 textColor = useBlackText
		? ImVec4(0.f, 0.f, 0.f, 1.f)
		: colors::input::text_active;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, framePadY));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::input::input_bg);
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors::input::input_bg);
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors::input::input_bg);
	ImGui::PushStyleColor(ImGuiCol_Border, colors::input::input_image);
	ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, selectionBg);
	ImGui::PushStyleColor(ImGuiCol_Text, textColor);

	ImGui::SetNextItemWidth(drawWidth);
	ImGui::PushItemWidth(drawWidth);
	const bool changed = std::forward<F>(drawField)();
	ImGui::PopItemWidth();
	ImGui::PopStyleColor(6);
	ImGui::PopStyleVar(2);

	ImGui::Dummy(ImVec2(0.f, 2.f));

	ImGui::PopID();

	return changed;
}

}

bool ksd::InputFloat(const char* label, float* value) noexcept
{
	return InputRowImpl(label, value, "##ksd_ifb_value", [value]()
	{
		return ImGui::InputFloat(
			"##ksd_ifb_value", value, 0.0f, 0.0f, "%.2f",
			ImGuiInputTextFlags_CharsDecimal);
	});
}

bool ksd::InputInt(const char* label, int* value) noexcept
{
	return InputRowImpl(label, value, "##ksd_ifb_int", [value]()
	{
		return ImGui::InputInt("##ksd_ifb_int", value, 0, 0, 0);
	});
}

bool ksd::InputTextOnly(const char* id, char* buf, const size_t buf_size, const float width, const ImGuiInputTextFlags flags) noexcept
{
	if (!id || !buf || buf_size == 0 || width < 36.f)
	{
		return false;
	}

	ImGui::PushID(id);

	constexpr float kFieldDrawOffsetX = -9.f;
	const ImVec2 cursorBefore(ImGui::GetCursorPos());
	ImGui::SetCursorPos(ImVec2(cursorBefore.x + kFieldDrawOffsetX, cursorBefore.y));

	constexpr float rowHeight = 30.f;
	const float framePadY = std::max(4.f, (rowHeight - ImGui::GetFontSize()) * 0.5f);

	static constexpr const char* kValueId = "##ksd_ito_value";
	ImGuiContext& g = *GImGui;
	const ImGuiID valueId = ImGui::GetID(kValueId);
	ImGuiInputTextState* const textState = ImGui::GetInputTextState(valueId);
	const bool useBlackText =
		g.ActiveId == valueId &&
		textState != nullptr &&
		textState->HasSelection();

	ImVec4 selectionBg = colors::accent_color;
	selectionBg.w *= 0.35f;

	const ImVec4 textColor = useBlackText
		? ImVec4(0.f, 0.f, 0.f, 1.f)
		: colors::input::text_active;

	ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6.f, framePadY));
	ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 5.f);
	ImGui::PushStyleColor(ImGuiCol_FrameBg, colors::input::input_bg);
	ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, colors::input::input_bg);
	ImGui::PushStyleColor(ImGuiCol_FrameBgActive, colors::input::input_bg);
	ImGui::PushStyleColor(ImGuiCol_Border, colors::input::input_image);
	ImGui::PushStyleColor(ImGuiCol_TextSelectedBg, selectionBg);
	ImGui::PushStyleColor(ImGuiCol_Text, textColor);

	ImGui::SetNextItemWidth(width);
	ImGui::PushItemWidth(width);
	const bool changed = ImGui::InputText(kValueId, buf, buf_size, flags);
	ImGui::PopItemWidth();
	ImGui::PopStyleColor(6);
	ImGui::PopStyleVar(2);

	ImGui::Dummy(ImVec2(0.f, 2.f));

	ImGui::PopID();

	return changed;
}
