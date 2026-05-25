#include "pages.hpp"

#include "../../dismay/config.h"
#include "../../dismay/functions.hpp"
#include "../../game/iw4hooks.h"
#include "../../game/functions.hpp"

#include <string>

namespace menu_pages {
	void RenderAboutPage() noexcept {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 3.f));
		ImGui::BeginGroup();
		{
			const ImVec2 origin(ImGui::GetCursorPos());
			const float leftX = origin.x - layout::leftPull;
			const float rightX = origin.x + layout::colWidth + layout::colGap;

			ImGui::SetCursorPos(ImVec2(leftX, origin.y));
			if(ksd::BeginChild(ICON_FA_CROSSHAIRS, "Config", 150.f, layout::leftWidth))
			{
				ksd::Text(".ini file is located in the same directory");
				ksd::Text("as the DLL.");
				if(ksd::Button("Load Config", ImVec2(280.f, 30.f)))
				{
					config::Load();
				}
				if(ksd::Button("Save Config", ImVec2(280.f, 30.f)))
				{
					config::Save();
				}
				ImGui::ColorEdit4("Accent Color", (float*)&colors::accent_color, ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);


			}

			ksd::EndChild();

			ImGui::SetCursorPos(ImVec2(rightX, origin.y));
			if(ksd::BeginChild(ICON_FA_KEYBOARD, "Links & Credits", 190.f, layout::rightWidth))
			{
				ksd::Text("MAJOR Thanks to:");
				ksd::TextLinkOpenURL("KingsleydotDev", "https://github.com/KingsleydotDev");
				ksd::TextLinkOpenURL("GRIIM", "https://x.com/GRIIMtB");
				ksd::Text("Founder (for suggestions and testing)");
				ImGui::Separator();
				ksd::Text("Check out my GitHub here: ");
				ksd::TextLinkOpenURL("0xKale", "https://github.com/0xKale");
				ImGui::Separator();
				ksd::TextLinkOpenURL("My fav anime", "https://www.youtube.com/watch?v=LkNechrBIrg");
			}
			ksd::EndChild();
		}
		ImGui::EndGroup();
		ImGui::PopStyleVar();
	}
}
