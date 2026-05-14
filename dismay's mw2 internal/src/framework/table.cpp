#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "table.hpp"
#include "draw_helpers.hpp"
#include "settings.h"

#include "../../ext/imgui/imgui.h"

#include <cstdarg>
#include <cstdio>

namespace ksd {

	void TableCellText(const char* const text) noexcept
	{
		ImGui::PushStyleColor(ImGuiCol_Text, colors::combo::text_inactive);
		ImGui::TextUnformatted(text ? text : "");
		ImGui::PopStyleColor();
	}

	void TableCellTextf(const char* const fmt, ...) noexcept
	{
		va_list args;
		va_start(args, fmt);
		ImGui::PushStyleColor(ImGuiCol_Text, colors::combo::text_inactive);
		ImGui::TextV(fmt, args);
		ImGui::PopStyleColor();
		va_end(args);
	}

	void TableCellTextColored(const ImVec4& color, const char* const text) noexcept
	{
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextUnformatted(text ? text : "");
		ImGui::PopStyleColor();
	}

	void TableCellTextfColored(const ImVec4& color, const char* const fmt, ...) noexcept
	{
		va_list args;
		va_start(args, fmt);
		ImGui::PushStyleColor(ImGuiCol_Text, color);
		ImGui::TextV(fmt, args);
		ImGui::PopStyleColor();
		va_end(args);
	}

	void TableCellGametype4(const char* const gametype) noexcept
	{
		char buf[5];
		std::snprintf(buf, sizeof(buf), "%.4s", gametype ? gametype : "");
		TableCellText(buf);
	}

	void TableCellPlayers(const int current, const int total) noexcept
	{
		ImVec4 col = colors::combo::text_inactive;
		if (total > 0 && current >= total)
		{
			col = ImVec4(1.f, 0.3f, 0.3f, 1.f);
		}
		else if (total > 0 && current * 3 >= total * 2)
		{
			col = ImVec4(1.f, 0.9f, 0.3f, 1.f);
		}
		else
		{
			col = ImVec4(0.3f, 1.f, 0.3f, 1.f);
		}
		ImGui::PushStyleColor(ImGuiCol_Text, col);
		ImGui::Text("%d/%d", current, total);
		ImGui::PopStyleColor();
	}

	void TableCellErrorf(const char* const fmt, ...) noexcept
	{
		const ImVec4 red(1.f, 0.4f, 0.4f, 1.f);
		va_list args;
		va_start(args, fmt);
		ImGui::PushStyleColor(ImGuiCol_Text, red);
		ImGui::TextV(fmt, args);
		ImGui::PopStyleColor();
		va_end(args);
	}

	void TableRowHoverAccent(const bool hovered, const float accentAlphaMul) noexcept
	{
		if (!hovered)
		{
			return;
		}
		const ImU32 packed = ksd::ColorWithAlpha(colors::accent_color, accentAlphaMul);
		ImGui::TableSetBgColor(ImGuiTableBgTarget_RowBg0, packed);
	}

}
