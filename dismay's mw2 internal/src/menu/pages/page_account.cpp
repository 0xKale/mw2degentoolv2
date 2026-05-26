#include "pages.hpp"

#include "../../../ext/imgui/imgui.h"
#include "../../../ext/imgui/imgui_internal.h"
#include "../../../ext/fonts/iconsfontawesome/IconsFontAwesome6.h"

#include "../../framework/framework.hpp"
#include "../../dismay/functions.hpp"
#include "../../game/iw4hooks.h"

namespace vars{
	int prestige = 10;
	int rank = 70;
	int wins = 0;
	int losses = 0;
	int ties = 0;
	int winStreak = 0;
	int kills = 0;
	int headshots = 0;
	int assists = 0;
	int killStreak = 0;
	int deaths = 0;
	int timePlayed = 0;
}

namespace menu_pages {
	void RenderAccountPage() noexcept {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 3.f));
		ImGui::BeginGroup();
		{
			const ImVec2 origin(ImGui::GetCursorPos());
			const float leftX = origin.x - layout::leftPull;
			const float rightX = origin.x + layout::colWidth + layout::colGap;

			ImGui::SetCursorPos(ImVec2(leftX, origin.y));
			ksd::BeginChild(ICON_FA_EYE, "Account", 245.f, layout::leftWidth);
			{
				ksd::SliderInt("Prestige:", &vars::prestige, 0, 11);
				ksd::SliderInt("Rank:", &vars::rank, 1, 70);
				ImGui::Dummy(ImVec2(0.f, 10.f));
				if(ksd::Button("Send Prestige",ImVec2(280.f, 30.f)))
				{
					functions::sendPrestige(vars::prestige);
					SendNotify("Prestige Sent", 2.0f);
				}
				if(ksd::Button("Send Rank",ImVec2(280.f, 30.f)))
				{
					functions::sendRank();
					SendNotify("Rank Sent", 2.0f);
				}

				if(ksd::Button("Unlock All",ImVec2(280.f, 30.f)))
				{
				functions::unlockAll(); // Mix client and true unlock. A bit of a hack but yolo
				functions::sendUnlockAllClients();
				SendNotify("Unlock All. Done.", 2.0f);
				}
				if (ImGui::IsItemHovered())
					ImGui::SetTooltip("Toggle in private match for golden spinning skull");
				if(ksd::Button("Gold Deagle Classes",ImVec2(280.f, 30.f)))
				{
					functions::sendGoldDeagleClasses();
					SendNotify("Gold Deagle Classes Added", 2.0f);
				}
			}
			ksd::EndChild();

			ImGui::SetCursorPos(ImVec2(rightX, origin.y));
			ksd::BeginChild(ICON_FA_EYE, "Profile Status", 460.f, layout::rightWidth);
			{
				ksd::InputInt("Wins:", &vars::wins);
				ksd::InputInt("Losses:", &vars::losses);
				ksd::InputInt("Ties:", &vars::ties);
				ksd::InputInt("Win Streak", &vars::winStreak);
				ksd::InputInt("Kills:", &vars::kills);
				ksd::InputInt("Headshots:", &vars::headshots);
				ksd::InputInt("Assists:", &vars::assists);
				ksd::InputInt("Kill Streak:", &vars::killStreak);
				ksd::InputInt("Deaths:", &vars::deaths);
				ksd::InputInt("Time Played:", &vars::timePlayed);
				if(ksd::Button("Load Profile Stats", ImVec2(280.f, 30.f)))
				{
					functions::loadProfileStats();
					SendNotify("Profile Loaded", 2.0f);
				}
				if(ksd::Button("Send Profile Stats", ImVec2(280.f, 30.f)))
				{
					functions::sendProfileStats();
					SendNotify("Stats Sent to Profile", 2.0f);
				}
			}
			ksd::EndChild();
		}
		ImGui::EndGroup();
		ImGui::PopStyleVar();
	}
}
