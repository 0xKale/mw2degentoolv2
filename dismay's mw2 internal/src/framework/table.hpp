#pragma once

#include "../../ext/imgui/imgui.h"

namespace ksd {

void TableCellText(const char* text) noexcept;
void TableCellTextf(const char* fmt, ...) IM_FMTARGS(1) noexcept;
void TableCellTextColored(const ImVec4& color, const char* text) noexcept;
void TableCellTextfColored(const ImVec4& color, const char* fmt, ...) IM_FMTARGS(2) noexcept;
void TableCellGametype4(const char* gametype) noexcept;
void TableCellPlayers(int current, int total) noexcept;
void TableCellErrorf(const char* fmt, ...) IM_FMTARGS(1) noexcept;

void TableRowHoverAccent(bool hovered, float accentAlphaMul = 0.1f) noexcept;

}
