#include "crosshair.hpp"
#include "../ext/imgui/imgui.h"
#include "configs_corrupted.h"
#include "crosshair_scope.hpp"

namespace corrupted
{
    void DrawCrosshairOverlay()
    {
        if (!configs.espCrosshair.current.enabled)
            return;

        if (crosshair_hud::g_scopeFlags.isScoped)
            return;

        auto& io = ImGui::GetIO();
        auto* dl = ImGui::GetBackgroundDrawList();
        const auto col = ImGui::GetColorU32(configs.colorCrossHair.current.color);
        const auto cx  = io.DisplaySize.x / 2.0f;
        const auto cy  = io.DisplaySize.y / 2.0f;

        dl->AddLine({ cx - 12.0f, cy - 1.0f }, { cx -  4.0f, cy - 1.0f }, col);
        dl->AddLine({ cx +  3.0f, cy - 1.0f }, { cx + 11.0f, cy - 1.0f }, col);
        dl->AddLine({ cx -  1.0f, cy - 12.0f }, { cx - 1.0f, cy -  4.0f }, col);
        dl->AddLine({ cx -  1.0f, cy +  3.0f }, { cx - 1.0f, cy + 11.0f }, col);
    }
}
