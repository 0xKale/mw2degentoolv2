#pragma once

#include "../../ext/imgui/imgui.h"

namespace ksd {

void Text(const char* fmt, ...) IM_FMTARGS(1);
void TextV(const char* fmt, va_list args) IM_FMTLIST(1);

bool TextLinkOpenURL(const char* label, const char* url = nullptr);

}
