#define IMGUI_DEFINE_MATH_OPERATORS
#include "combo.hpp"
#include "framework.hpp"

#include "../../ext/imgui/imgui.h"
#include "../../ext/imgui/imgui_internal.h"

#include <cfloat>
#include <map>
#include <string>
#include <vector>

using namespace ImGui;

namespace {

int g_beginComboDepth = 0;

}

namespace ksd {

    static float CalcMaxPopupHeightFromItemCount(int items_count)
    {
        ImGuiContext& g = *GImGui;
        if (items_count <= 0)
            return FLT_MAX;
        return (g.FontSize + g.Style.ItemSpacing.y) * items_count - g.Style.ItemSpacing.y + (g.Style.WindowPadding.y * 2);
    }

    static void RenderTextColor(ImFont* font, const ImVec2& p_min, const ImVec2& p_max, ImU32 col, const char* text, const ImVec2& align)
    {
        PushFont(font);
        PushStyleColor(ImGuiCol_Text, col);
        RenderTextClipped(p_min, p_max, text, NULL, NULL, align, NULL);
        PopStyleColor();
        PopFont();
    }

    struct ComboAnim
    {
        ImVec4 background, text, outline;
        float open, alpha, comboSize = 0.f, shadowAlpha;
        bool isOpen = false, hovered = false;
        float arrowRoll;
        float textAlpha;
    };

    static const char* ItemsArrayGetter(void* data, int idx)
    {
        const char* const* items = (const char* const*)data;
        return items[idx];
    }

    bool BeginCombo(const char* label, const char* preview_value, int val, bool multi, ImGuiComboFlags flags)
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = GetCurrentWindow();

        g.NextWindowData.ClearFlags();
        if (window->SkipItems) return false;

        const ImGuiStyle& style = g.Style;
        const ImGuiID id = window->GetID(label);
        const ImVec2 label_size = CalcTextSize(label, NULL, true);
        const float w = ((GetContentRegionMax().x - style.WindowPadding.x));

        const ImRect bb(window->DC.CursorPos + ImVec2(0, 0), window->DC.CursorPos + ImVec2(w + 8, 30));
        const ImRect rect(window->DC.CursorPos + ImVec2(140, 0), window->DC.CursorPos + ImVec2(w + 8, 30));

        const ImRect total_bb(bb.Min, bb.Max);
        ItemSize(ImRect(total_bb.Min, total_bb.Max - ImVec2(0, 15)));

        if (!ItemAdd(bb, id, &bb)) return false;

        bool hovered, held, pressed = ButtonBehavior(bb, id, &hovered, &held);

        static std::map<ImGuiID, ComboAnim> anim;
        ComboAnim& state = anim[id];

        if ((hovered && g.IO.MouseClicked[0]) || (state.isOpen && g.IO.MouseClicked[0] && !state.hovered))
            state.isOpen = !state.isOpen;

        state.arrowRoll = ImLerp(state.arrowRoll, state.isOpen ? -1.f : 1.f, g.IO.DeltaTime * 6.f);
        state.textAlpha = ImLerp(state.textAlpha, state.isOpen ? 1.f : 0.3f, g.IO.DeltaTime * 6.f);
        state.text = ImLerp(state.text, state.isOpen ? colors::combo::text_active : hovered ? colors::combo::text_hovered : colors::combo::text_inactive, g.IO.DeltaTime * 6.f);
        const float targetOpenHeight = (ImMax(val, 1) * 33.f) + 5.f;
        state.comboSize = ImLerp(state.comboSize, state.isOpen ? targetOpenHeight : 0.f, g.IO.DeltaTime * 12.f);

        GetWindowDrawList()->AddRectFilled(rect.Min, rect.Max, GetColorU32(colors::combo::combo_bg), 4);

        RenderTextColor(
            fonts::combo_icon_font,
            bb.Min + ImVec2(w - 20, 6),
            bb.Min + ImVec2(w, 20),
            ksd::ColorWithAlpha(colors::accent_color, state.textAlpha),
            "z",
            ImVec2(1.f, 0.5f));
        RenderTextColor(fonts::inter_bold_font2, rect.Min + ImVec2(10, 1), rect.Min + ImVec2(100, 30), GetColorU32(state.text), preview_value, ImVec2(0.f, 0.5f));
        RenderTextColor(fonts::inter_bold_font2, bb.Min + ImVec2(-7, 6), bb.Max, GetColorU32(state.text), label, ImVec2(0.f, 0.2f));

        if (!IsRectVisible(rect.Min, rect.Max + ImVec2(0, 2)))
        {
            state.isOpen = false;
            state.comboSize = 0.f;
        }

        if (!state.isOpen && state.comboSize < 2.f) return false;

        SetNextWindowPos(ImVec2(rect.Min.x, rect.Max.y + 5));
        SetNextWindowSize(ImVec2(rect.GetWidth(), state.comboSize));

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoFocusOnAppearing;

        PushStyleColor(ImGuiCol_WindowBg, colors::combo::combo_bg);
        PushStyleColor(ImGuiCol_Border, colors::combo::combo_bg);
        PushStyleVar(ImGuiStyleVar_WindowRounding, 4);
        PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
        PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.f);

        Begin(label, NULL, window_flags);
        ++g_beginComboDepth;

        PopStyleVar(3);
        PopStyleColor(2);

        state.hovered = IsWindowHovered() || IsWindowHovered(ImGuiHoveredFlags_ChildWindows);

        if (multi && state.hovered && g.IO.MouseClicked[0]) state.isOpen = false;

        return true;
    }

    void EndCombo()
    {
        if (g_beginComboDepth > 0)
        {
            End();
            --g_beginComboDepth;
        }
    }

    bool SelectableListCombo(
        const char* label,
        int* current_item,
        const char* const items[],
        const int items_count,
        const int visible_rows,
        const float accent_hover_alpha_mul)
    {
        if (!label || !current_item || !items || items_count <= 0 || visible_rows < 1)
        {
            return false;
        }

        if (*current_item < 0 || *current_item >= items_count)
        {
            *current_item = ImClamp(*current_item, 0, items_count - 1);
        }

        const char* const preview = items[*current_item];
        bool value_changed = false;

        if (BeginCombo(label, preview, visible_rows, false, 0))
        {
            const ImVec4 transparent(0.f, 0.f, 0.f, 0.f);
            const ImU32 accentHoverPacked = ksd::ColorWithAlpha(colors::accent_color, accent_hover_alpha_mul);
            const ImVec4 accentHover = ColorConvertU32ToFloat4(accentHoverPacked);

            PushStyleColor(ImGuiCol_Header, transparent);
            PushStyleColor(ImGuiCol_HeaderHovered, accentHover);
            PushStyleColor(ImGuiCol_HeaderActive, accentHover);
            PushStyleColor(ImGuiCol_NavCursor, transparent);
            PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15.f, 15.f));

            const float rowH = GetTextLineHeightWithSpacing();
            const float listClipH = rowH * static_cast<float>(visible_rows);

            PushID(label);
            ImGui::BeginChild(
                "list",
                ImVec2(-FLT_MIN, listClipH),
                ImGuiChildFlags_None,
                ImGuiWindowFlags_NoScrollbar);

            ImFont* const fontRegular = fonts::inter_font ? fonts::inter_font : GetFont();
            ImFont* const fontBold = fonts::inter_bold_font2 ? fonts::inter_bold_font2 : fontRegular;

            for (int i = 0; i < items_count; ++i)
            {
                PushID(i);
                const bool selected = (*current_item == i);
                PushFont(selected ? fontBold : fontRegular);
                const float rowW = GetContentRegionAvail().x;
                if (Selectable(items[i], selected, 0, ImVec2(rowW, 0.f)))
                {
                    *current_item = i;
                    value_changed = true;
                }
                if (selected)
                {
                    SetItemDefaultFocus();
                }
                PopFont();
                PopID();
            }

            ImGui::EndChild();
            PopID();

            PopStyleVar();
            PopStyleColor(4);
        }

        EndCombo();
        Dummy(ImVec2(0.f, 15.f));
        return value_changed;
    }

    void MultiCombo(const char* label, bool variable[], const char* labels[], int count)
    {
        ImGuiContext& g = *GImGui;

        std::string preview = "None";

        for (auto i = 0, j = 0; i < count; i++)
        {
            if (variable[i])
            {
                if (j)
                    preview += (", ") + (std::string)labels[i];
                else
                    preview = labels[i];

                j++;
            }
        }

        if (BeginCombo(label, preview.c_str(), count, NULL, NULL))
        {
            for (auto i = 0; i < count; i++)
            {
                PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 15));
                PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 15));
                Selectable(labels[i], &variable[i], ImGuiSelectableFlags_DontClosePopups);
                PopStyleVar(2);
            }
            EndCombo();
        }

        preview = ("None");
    }

    static bool BeginComboPreview()
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiComboPreviewData* preview_data = &g.ComboPreviewData;

        if (window->SkipItems || !(g.LastItemData.StatusFlags & ImGuiItemStatusFlags_Visible)) return false;

        IM_ASSERT(g.LastItemData.Rect.Min.x == preview_data->PreviewRect.Min.x && g.LastItemData.Rect.Min.y == preview_data->PreviewRect.Min.y);

        if (!window->ClipRect.Overlaps(preview_data->PreviewRect)) return false;

        preview_data->BackupCursorPos = window->DC.CursorPos;
        preview_data->BackupCursorMaxPos = window->DC.CursorMaxPos;
        preview_data->BackupCursorPosPrevLine = window->DC.CursorPosPrevLine;
        preview_data->BackupPrevLineTextBaseOffset = window->DC.PrevLineTextBaseOffset;
        preview_data->BackupLayout = window->DC.LayoutType;
        window->DC.CursorPos = preview_data->PreviewRect.Min + g.Style.FramePadding;
        window->DC.CursorMaxPos = window->DC.CursorPos;
        window->DC.LayoutType = ImGuiLayoutType_Horizontal;
        window->DC.IsSameLine = false;
        PushClipRect(preview_data->PreviewRect.Min, preview_data->PreviewRect.Max, true);

        return true;
    }

    static void EndComboPreview()
    {
        ImGuiContext& g = *GImGui;
        ImGuiWindow* window = g.CurrentWindow;
        ImGuiComboPreviewData* preview_data = &g.ComboPreviewData;

        ImDrawList* draw_list = window->DrawList;
        if (window->DC.CursorMaxPos.x < preview_data->PreviewRect.Max.x && window->DC.CursorMaxPos.y < preview_data->PreviewRect.Max.y)
            if (draw_list->CmdBuffer.Size > 1)
            {
                draw_list->_CmdHeader.ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 1].ClipRect = draw_list->CmdBuffer[draw_list->CmdBuffer.Size - 2].ClipRect;
                draw_list->_TryMergeDrawCmds();
            }
        PopClipRect();
        window->DC.CursorPos = preview_data->BackupCursorPos;
        window->DC.CursorMaxPos = ImMax(window->DC.CursorMaxPos, preview_data->BackupCursorMaxPos);
        window->DC.CursorPosPrevLine = preview_data->BackupCursorPosPrevLine;
        window->DC.PrevLineTextBaseOffset = preview_data->BackupPrevLineTextBaseOffset;
        window->DC.LayoutType = preview_data->BackupLayout;
        window->DC.IsSameLine = false;
        preview_data->PreviewRect = ImRect();
    }

    static const char* ItemsSingleStringGetter(void* data, int idx)
    {
        const char* items_separated_by_zeros = (const char*)data;
        int items_count = 0;
        const char* p = items_separated_by_zeros;
        while (*p)
        {
            if (idx == items_count)
                break;
            p += strlen(p) + 1;
            items_count++;
        }
        return *p ? p : NULL;
    }

    bool Combo(const char* label, int* current_item, const char* (*getter)(void* user_data, int idx), void* user_data, int items_count, int popup_max_height_in_items)
    {
        ImGuiContext& g = *GImGui;

        const char* preview_value = NULL;
        if (*current_item >= 0 && *current_item < items_count)
            preview_value = getter(user_data, *current_item);

        if (popup_max_height_in_items != -1 && !(g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint))
            SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(popup_max_height_in_items)));

        if (!BeginCombo(label, preview_value, items_count, ImGuiComboFlags_None, NULL)) return false;

        bool value_changed = false;
        PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(15, 15));
        for (int i = 0; i < items_count; i++)
        {
            const char* item_text = getter(user_data, i);
            if (item_text == NULL)
                item_text = "*Unknown item*";

            PushID(i);
            const bool item_selected = (i == *current_item);
            if (Selectable(item_text, item_selected) && *current_item != i)
            {
                value_changed = true;
                *current_item = i;
            }
            if (item_selected)
                SetItemDefaultFocus();
            PopID();
        }
        PopStyleVar();

        EndCombo();

        if (value_changed)
            MarkItemEdited(g.LastItemData.ID);

        return value_changed;
    }

    bool Combo(const char* label, int* current_item, const char* const items[], int items_count, int height_in_items)
    {
        const bool value_changed = Combo(label, current_item, ItemsArrayGetter, (void*)items, items_count, height_in_items);
        return value_changed;
    }

    bool Combo(const char* label, int* current_item, const char* items_separated_by_zeros, int height_in_items)
    {
        int items_count = 0;
        const char* p = items_separated_by_zeros;
        while (*p)
        {
            p += strlen(p) + 1;
            items_count++;
        }
        bool value_changed = Combo(label, current_item, ItemsSingleStringGetter, (void*)items_separated_by_zeros, items_count, height_in_items);
        return value_changed;
    }

}
