#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "child.hpp"
#include "settings.h"

#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"

namespace {

constexpr float headerHeight = 40.f;
constexpr float contentPadTop = 50.f;

static bool BeginChildEx(
	const char* icon,
	const char* title,
	ImGuiID childId,
	const ImVec2& outerSize,
	bool skipHeader,
	ImGuiWindowFlags extraFlags)
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* parent = g.CurrentWindow;

	ImGuiWindowFlags windowFlags = extraFlags;
	windowFlags |= ImGuiWindowFlags_ChildWindow | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
		ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBackground;
	windowFlags |= (parent->Flags & ImGuiWindowFlags_NoMove);

	const ImVec2 avail = ImGui::GetContentRegionAvail();
	ImVec2 resolved = ImFloor(outerSize);

	if (resolved.x <= 0.0f)
	{
		resolved.x = ImMax(avail.x + resolved.x, 4.0f);
	}

	if (resolved.y <= 0.0f)
	{
		resolved.y = ImMax(avail.y + resolved.y, 4.0f);
	}

	ImGui::SetNextWindowSize(resolved);

	if (!skipHeader)
	{
		const ImVec2 headerOrigin = parent->DC.CursorPos;
		const float chromeWidth = resolved.x + 0.2f;

		parent->DrawList->AddRectFilled(
			headerOrigin,
			headerOrigin + ImVec2(chromeWidth, resolved.y),
			ImGui::GetColorU32(colors::child::child_background),
			colors::child::child_rounding,
			ImDrawFlags_None);

		parent->DrawList->AddRectFilled(
			headerOrigin,
			headerOrigin + ImVec2(chromeWidth, headerHeight),
			ImGui::GetColorU32(colors::child::child_top),
			colors::child::child_rounding,
			ImDrawFlags_RoundCornersTop);

		if (icon != nullptr && icon[0] != '\0')
		{
			ImFont* iconFont = fonts::fa_font;

			if (!iconFont)
			{
				iconFont = ImGui::GetFont();
			}

			const char* const glyphEnd = ImGui::FindRenderedTextEnd(icon);

			parent->DrawList->AddText(
				iconFont, 18.f,
				headerOrigin + ImVec2(11.f, 13.f),
				ImGui::GetColorU32(colors::accent_color),
				icon, glyphEnd, 0.f, nullptr);
		}

		ImFont* titleFont = fonts::inter_bold_font3;

		if (!titleFont)
		{
			titleFont = ImGui::GetFont();
		}

		if (title)
		{
			parent->DrawList->AddText(
				titleFont, 18.f,
				headerOrigin + ImVec2(38.f, 12.f),
				ImGui::GetColorU32(colors::accent_color),
				title);
		}
	}

	const ImGuiChildFlags childFlags = ImGuiChildFlags_AlwaysUseWindowPadding;

	return ImGui::BeginChildEx(title, childId, ImVec2(0.0f, 0.0f), childFlags, windowFlags);
}

}

bool ksd::BeginChild(const char* icon, const char* id, float contentHeight, float width)
{
	ImGuiWindow* window = ImGui::GetCurrentWindow();

	if (width <= 0.f)
	{
		width = settings::menu_main_column_width_px;
	}

	const float outerHeight = contentHeight + contentPadTop;

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(20, contentPadTop));
	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 3));

	ImGuiID childId = window->GetID(id);

	ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse;

	return ::BeginChildEx(icon, id, childId, ImVec2(width, outerHeight), false, flags);
}

void ksd::EndChild()
{
	ImGuiContext& g = *GImGui;
	ImGuiWindow* childWindow = g.CurrentWindow;

	ImGui::PopStyleVar(2);

	const ImGuiID backupEndChildId = g.WithinEndChildID;
	IM_ASSERT(childWindow->Flags & ImGuiWindowFlags_ChildWindow);

	g.WithinEndChildID = childWindow->ID;
	ImVec2 childSize = childWindow->Size;
	ImGui::End();

	if (childWindow->BeginCount == 1)
	{
		ImGuiWindow* parent = g.CurrentWindow;
		ImRect bb(parent->DC.CursorPos, parent->DC.CursorPos + childSize);
		ImGui::ItemSize(childSize);
		const bool navFlattened = (childWindow->ChildFlags & ImGuiChildFlags_NavFlattened) != 0;

		if ((childWindow->DC.NavLayersActiveMask != 0 || childWindow->DC.NavWindowHasScrollY) && !navFlattened)
		{
			ImGui::ItemAdd(bb, childWindow->ChildId);
			ImGui::RenderNavCursor(bb, childWindow->ChildId);

			if (childWindow->DC.NavLayersActiveMask == 0 && childWindow == g.NavWindow)
			{
				const ImRect highlight(bb.Min - ImVec2(2.f, 2.f), bb.Max + ImVec2(2.f, 2.f));
				ImGui::RenderNavCursor(highlight, g.NavId, ImGuiNavRenderCursorFlags_Compact);
			}
		}
		else
		{
			ImGui::ItemAdd(bb, childWindow->ChildId, NULL, ImGuiItemFlags_NoNav);

			if (navFlattened)
			{
				parent->DC.NavLayersActiveMaskNext |= childWindow->DC.NavLayersActiveMaskNext;
			}
		}

		if (g.HoveredWindow == childWindow)
		{
			g.LastItemData.StatusFlags |= ImGuiItemStatusFlags_HoveredWindow;
		}

		childWindow->DC.ChildItemStatusFlags = g.LastItemData.StatusFlags;
	}
	else
	{
		ImGui::SetLastItemData(childWindow->ChildId, g.CurrentItemFlags, childWindow->DC.ChildItemStatusFlags, childWindow->Rect());
	}

	g.WithinEndChildID = backupEndChildId;
	g.LogLinePosY = -FLT_MAX;

	ImGui::Spacing();
}
