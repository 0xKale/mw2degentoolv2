#pragma once

#include "../../ext/imgui/imgui.h"
#include "../../ext/fonts/iconsfontawesome/IconsFontAwesome6.h"

namespace colors {
    inline ImVec4 accent_color = ImColor(255, 255, 255);

    namespace menu {
        inline ImVec4 window_bg = ImColor(20, 20, 22);
        inline ImVec4 border = ImColor(17, 17, 17, 0);
        inline ImVec4 watermark_bg = ImColor(21, 19, 20, 100);
        inline ImVec4 watermark_border = ImColor(21, 19, 20, 0);
        inline ImVec4 watermark_filled = ImColor(27, 25, 28);
    }

    namespace tabs {
        inline ImVec4 text_active = ImColor(255, 255, 255);
        inline ImVec4 text_hovered = ImColor(150, 150, 150);
        inline ImVec4 text_inactive = ImColor(76, 76, 77);
    }

    namespace child {
        inline ImVec4 child_background = ImColor(23, 23, 25);
        inline ImVec4 child_top = ImColor(25, 25, 27);
        inline float child_rounding = 7.f;
    }

    namespace checkbox {
        inline ImVec4 text_active = ImColor(255, 255, 255);
        inline ImVec4 text_hovered = ImColor(150, 150, 150);
        inline ImVec4 text_inactive = ImColor(76, 76, 77);
        inline ImVec4 checkbox_bg_active = ImColor(37, 37, 39);
        inline ImVec4 checkbox_bg_inactive = ImColor(35, 35, 37);
        inline ImVec4 circle_inactive = ImColor(44, 44, 46);
    }

    namespace slider {
        inline ImVec4 text_active = ImColor(255, 255, 255);
        inline ImVec4 text_hovered = ImColor(150, 150, 150);
        inline ImVec4 text_inactive = ImColor(76, 76, 77);
        inline ImVec4 slider_inactive = ImColor(35, 35, 37);
    }

    namespace combo {
        inline ImVec4 text_active = ImColor(255, 255, 255);
        inline ImVec4 text_hovered = ImColor(150, 150, 150);
        inline ImVec4 text_inactive = ImColor(76, 76, 77);
        inline ImVec4 combo_bg = ImColor(27, 27, 29);
    }

    namespace color_picker {
        inline ImVec4 picker_bg = ImColor(27, 27, 29);
    }

    namespace binder {
        inline ImVec4 binder_bg = ImColor(27, 27, 29);
        inline ImVec4 text_active = ImColor(255, 255, 255);
        inline ImVec4 text_hovered = ImColor(150, 150, 150);
        inline ImVec4 text_inactive = ImColor(76, 76, 77);
        inline ImVec4 line = ImColor(40, 40, 42);
        inline ImVec4 image_active = ImColor(255, 255, 255);
        inline ImVec4 image_hovered = ImColor(150, 150, 150);
        inline ImVec4 image_inactive = ImColor(76, 76, 77);
    }

    namespace button {
        inline ImVec4 button_bg = ImColor(27, 27, 29);
        inline ImVec4 text_active = ImColor(255, 255, 255);
        inline ImVec4 text_inactive = ImColor(76, 76, 77);
    }

    namespace input {
        inline ImVec4 input_bg = ImColor(27, 27, 29);
        inline ImVec4 input_image = ImColor(47, 47, 49);
        inline ImVec4 text_active = ImColor(255, 255, 255);
        inline ImVec4 text_hovered = ImColor(150, 150, 150);
        inline ImVec4 text_inactive = ImColor(76, 76, 77);
    }

    namespace preview {
        inline ImVec4 rect = ImColor(40, 40, 42);
        inline ImVec4 name = ImColor(255, 255, 255);
        inline ImVec4 distance = ImColor(255, 255, 255);
        inline ImVec4 head = ImColor(255, 255, 255);
        inline ImVec4 dice = ImColor(255, 255, 255);
    }
}

namespace settings {
    inline constexpr float menu_main_column_width_px = 299.f;
    inline constexpr float menu_main_column_gap_px = 10.f;
    inline constexpr float menu_body_side_inset_px = 10.f;

    inline constexpr float menu_interior_body_width_px =
        menu_body_side_inset_px * 2.f + menu_main_column_width_px * 2.f + menu_main_column_gap_px;

    inline constexpr float menu_outer_margin_from_window_edge_px = 10.f;

    inline constexpr float menu_window_inner_width_px =
        menu_interior_body_width_px + menu_outer_margin_from_window_edge_px * 2.f;

    inline ImVec2 size_menu = ImVec2(menu_window_inner_width_px, 520.f);
    inline ImVec2 size_watermark = ImVec2(479, 50);
    inline ImVec2 size_preview = ImVec2(300, 400);
    inline float checkbox_rounding = 100.f;
}

namespace misc {
    inline int tab_count = 0, active_tab_count = 0;
    inline float anim_tab = 0;
    inline int tab_width = 85;
    inline float child_add = 0, alpha_child = 0;
}

namespace menu {
    inline ImVec4 general_child = ImColor(23, 23, 25);
}


namespace pictures {
    inline void* logo_img = nullptr;
    inline const char* aim_img = ICON_FA_CROSSHAIRS;
    inline const char* misc_img = ICON_FA_GEAR;
    inline const char* visual_img = ICON_FA_EYE;
    inline const char* keyboard_img = ICON_FA_KEYBOARD;
    inline void* silentaim_img = nullptr;
    inline void* trigger_img = nullptr;
    inline void* world_img = nullptr;
    inline void* settings_img = nullptr;
    inline void* pen_img = nullptr;
    inline void* input_img = nullptr;
    inline void* wat_logo_img = nullptr;
    inline void* fps_img = nullptr;
    inline void* player_img = nullptr;
    inline void* time_img = nullptr;
}

namespace fonts {
    inline ImFont* inter_font = nullptr;
    inline ImFont* inter_bold_font = nullptr;
    inline ImFont* inter_bold_font2 = nullptr;
    inline ImFont* inter_bold_font3 = nullptr;
    inline ImFont* inter_bold_font4 = nullptr;
    inline ImFont* inter_font_b = nullptr;
    inline ImFont* combo_icon_font = nullptr;
    inline ImFont* weapon_font = nullptr;
    inline ImFont* fa_font = nullptr;
    inline ImFont* morpheus_title = nullptr;
}

namespace features {
    inline bool check1 = false, check2 = false, check3 = false, check4 = false, check5 = false, check6 = false, check7 = false;
    inline int sliderint = 0, sliderint2 = 0, sliderint3 = 0, sliderint4 = 0;
    inline int selectedItem = 0, selected = 0;
    inline const char* items[]{ "Value", "Random" };
    inline const char* items_count[]{ "Combo1", "Combo2", "Combo3", "Combo4" };
    inline ImVec4 fov_color = ImColor(60, 157, 173);
    inline int key = 0, mind = 1;
    inline int key2 = 0, mind2 = 0;
    inline int key3 = 0, mind3 = 0;
    inline char input[64] = { "" };
    inline bool multi[5] = { false, true, true, false, true };
    inline const char* multi_items[5] = { "Head", "Chest", "Stromatch", "Body", "Legs" };
    inline bool multi_esp[7] = { false, false, false, false, false, false, false };
    inline const char* multi_preview[7] = { "Box", "Health", "Armor", "Nickname", "Distance", "Weapon", "Skeleton" };
    inline bool esp_perview = false;
    inline bool watermark = true;
    inline float preview_alpha = 0;
}
