#pragma once

#include "../ext/imgui/imgui.h"

namespace corrupted
{
    // Minimal config surface for the ImGui crosshair overlay only.
    struct configs_c
    {
        struct
        {
            struct
            {
                bool enabled = false;
            } current;
        } espCrosshair;

        struct
        {
            struct
            {
                //ImVec4 color = { 1.0f, 1.0f, 1.0f, 1.0f }; // White
                ImVec4 color = { 255.0f / 255.0f, 0.0f, 0.0f, 1.0f }; // Red
            } current;
        } colorCrossHair;
    };

    extern configs_c configs;
}
