#include "pages.hpp"

#include "../../dismay/functions.hpp"
#include "../../game/iw4hooks.h"

#include <string>

namespace vars {

	char playerName[18][64] = {
		"client0", "client1", "client2", "client3", "client4", "client5", "client6", "client7",
		"client8", "client9", "client10", "client11", "client12", "client13", "client14", "client15",
		"client16", "client17",
	};
	char serverCommand[256] = "";
	const char* mapList[26] = {"mp_afghan", "mp_derail", "mp_estate", "mp_favela", "mp_highrise",
		"mp_invasion", "mp_checkpoint", "mp_quarry", "mp_rundown", "mp_rust", "mp_boneyard", "mp_nightshift", "mp_subbase",
		"mp_terminal", "mp_underpass", "mp_brecourt", "mp_complex", "mp_crash", "mp_compact", "mp_overgrown", "mp_storm",
		"mp_abandon", "mp_fuel2", "mp_strike", "mp_trailerpark", "mp_vacant"};
	const char* mapListDisplay[26] = {
			"Afghan", "Derail", "Estate", "Favela", "Highrise", "Invasion", "Karachi",
			"Quarry", "Rundown", "Rust", "Scrapyard", "Skidrow", "Sub Base", "Terminal",
			"Underpass", "Wasteland", "Bailout", "Crash", "Salvage",
			"Overgrown", "Storm", "Carnival", "Fuel", "Strike", "Trailer Park", "Vacant"};
	int selectedMap = 0;
	const char* gamemodeListDisplay[12] = {
		"Domination", "Team Deathmatch", "Search and Destroy", "Free-For-All", "Headquarters", "Demolition", "Sabotage", "Capture the Flag", "Global Thermonuclear War", "One Flag CTF", "VIP", "Arena"};
	const char* gamemodeList[12] = {
		"dom", "war", "sd", "ffa", "koth", "dem", "sab", "ctf", "gtnw", "oneflag", "vip", "arena"};
	int selectedGamemode = 0;
	int maxPlayers = 18;
	bool FFATeamFix = false;
	float sprintScale = 1.0f;
	float knockbackScale = 1000.0f;
	float backSpeedScale = 0.69f;
	bool enableDepatchBounces = false;
	bool enableDepatchBouncesEasy = false;
	bool enableDepatchElevators = false;
}

namespace menu_pages {
	void RenderHostPage() noexcept {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 3.f));
		ImGui::BeginGroup();
		{
			const ImVec2 origin(ImGui::GetCursorPos());
			const float leftX = origin.x - layout::leftPull;
			const float rightX = origin.x + layout::colWidth + layout::colGap;
	
			ImGui::SetCursorPos(ImVec2(leftX, origin.y));
			if(ksd::BeginChild(ICON_FA_CROSSHAIRS, "Server", 420.f, 636))
			{
				ksd::Text("s = setClientDvar (Infects everyone in the lobby)");
				ImGui::Dummy(ImVec2(0.f, 1.f));
				ksd::Text("c = iPrintInBold (Puts Text In Center Of Screen, not permanent)");
				ImGui::Dummy(ImVec2(0.f, 1.f));
				ksd::Text("f = iPrintIn (Text Above Kill feed, not permanent)");
				ImGui::Dummy(ImVec2(0.f, 1.f));
				ksd::Text("J = setPlayerData (Allows You To Unlock Challenges, Sets Stats, etc.)");
				ImGui::Dummy(ImVec2(0.f, 1.f));
				ksd::Text("M = setVisionNaked (Sets on of the _mp visions)");
				ImGui::Dummy(ImVec2(0.f, 1.f));
				ksd::Text("n = player Volume?? (default is 1. 999+ is possible WARNING!!!)");
				ImGui::Dummy(ImVec2(0.f, 1.f));
				ksd::Text("Server Commands:");
				ImGui::Dummy(ImVec2(0.f, 1.f));
				ksd::InputTextOnly("##serverCommand", vars::serverCommand, sizeof(vars::serverCommand), 609.f);
				if(ksd::Button("Send to ALL clients", ImVec2(291.5f, 30.f)))
				{
					for (int i = 0; i <= 17; ++i) {
						SV_GameSendServerCommand(i, 0, (char*)vars::serverCommand);
					}

				}ImGui::SameLine();
				if(ksd::Button("Unlock ALL clients", ImVec2(291.5f, 30.f)))
				{

					functions::sendUnlockAllClients();
				}
				if(ksd::Button(vars::playerName[0], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[1], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[2], ImVec2(136.5f, 30.f)))
				{
					
				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[3], ImVec2(136.5f, 30.f)))
				{

				}
				if(ksd::Button(vars::playerName[4], ImVec2(136.f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[5], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[6], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[7], ImVec2(136.5f, 30.f)))
				{

				}
				if(ksd::Button(vars::playerName[8], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[9], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[10], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[11], ImVec2(136.5f, 30.f)))
				{

				}
				if(ksd::Button(vars::playerName[12], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[13], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[14], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[15], ImVec2(136.5f, 30.f)))
				{

				}
				if(ksd::Button(vars::playerName[16], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button(vars::playerName[17], ImVec2(136.5f, 30.f)))
				{

				}ImGui::SameLine();
				if(ksd::Button("Load Player Names", ImVec2(136.5f, 30.f)))
				{
					functions::loadPlayerNames();
				}
				
			}
			ksd::EndChild();

			ImGui::SetCursorPos(ImVec2(leftX, origin.y + 480.f));
			if(ksd::BeginChild(ICON_FA_CROSSHAIRS, "Hosting", 280.f, layout::leftWidth))
			{
				ksd::SelectableListCombo("Select Map", &vars::selectedMap, vars::mapListDisplay, 26);
				ksd::SelectableListCombo("Gamemode", &vars::selectedGamemode, vars::gamemodeListDisplay, 12);
				ksd::SliderInt("Max Players", &vars::maxPlayers, 1, 18);
				ImGui::Dummy(ImVec2(0.f, 10.f));
				if(ksd::Button("Cahnge Map", ImVec2(132.f, 30.f)))
				{
					functions::ChangeGamemode();
					Cbuf_AddCall(0, functions::ChangeMap);

				}ImGui::SameLine();
				if(ksd::Button("Fast Restart", ImVec2(132.f, 30.f)))
				{
					functions::FastRestart();
				}
				if(ksd::Button("Lock Lobby", ImVec2(132.f, 30.f)))
				{
					Cbuf_AddText(0, "g_password 666");
				}ImGui::SameLine();
				if(ksd::Button("Match Settings", ImVec2(132.f, 30.f)))
				{
					OpenMenu(0, "popup_gamesetup");
				}
				if(ksd::Button("Start Match", ImVec2(282.f, 30.f)))
				{
					functions::ChangeGamemode();
					functions::doMaxPlayers(vars::maxPlayers);
					functions::doStartMatch();
					functions::doBalanceTeams();
				}
				ImGui::Dummy(ImVec2(0.f, 4.f));
				ksd::Checkbox("FFA Team Fix", &vars::FFATeamFix);
			}
			ksd::EndChild();

			ImGui::SetCursorPos(ImVec2(rightX, origin.y + 480.f));
			if(ksd::BeginChild(ICON_FA_KEYBOARD, "Lobby Tweaks", 370.f, layout::rightWidth))
			{
				if(ksd::Button("Give Ammo", ImVec2(280.f, 30.f)))
				{
					functions::giveAmmo();
				}
				if(ksd::Button("Unlimited Time/Score", ImVec2(280.f, 30.f)))
				{
					Cbuf_AddText(0, "scr_dm_timelimit 0; scr_war_timelimit 0; scr_dom_timelimit 0");
					Cbuf_AddText(0, "scr_dm_scorelimit 0; scr_war_scorelimit 0; ""scr_dom_scorelimit 0");
				}
				if(ksd::SliderFloat("Sprint Scale", &vars::sprintScale, 1.0f, 30.0f))
				{
					functions::sendSprintScale();
				}
				ImGui::Dummy(ImVec2(0.f, 4.f));
				if(ksd::Button("360 Prone Cap Off", ImVec2(132.f, 30.f)))
				{
					Cbuf_AddText(0, "bg_prone_yawcap 360");
				}ImGui::SameLine();
				if(ksd::Button("360 Prone Cap On", ImVec2(132.f, 30.f)))
				{
				Cbuf_AddText(0, "bg_prone_yawcap 85");
			}
				if(ksd::Button("360 Ladder Cap Off", ImVec2(132.f, 30.f)))
				{
					Cbuf_AddText(0, "bg_ladder_yawcap 360");
				}ImGui::SameLine();
				if(ksd::Button("360 Ladder Cap On", ImVec2(132.f, 30.f)))
				{
					Cbuf_AddText(0, "bg_ladder_yawcap 100");
				}
				if(ksd::SliderFloat("Knockback Scale", &vars::knockbackScale, 1000, 999999))
				{
					functions::sendKnockbackScale();
				}
				if(ksd::SliderFloat("Back Speed Scale", &vars::backSpeedScale, 0.69f, 300.0f))
				{
					functions::sendBackSpeedScale();
				}
				if(ksd::Checkbox("Depatch Bounces", &vars::enableDepatchBounces))
				{
					if (vars::enableDepatchBounces)
					{
						vars::enableDepatchBouncesEasy = false;
						functions::sendBouncesToggle(); 
					}

				}
				if(ksd::Checkbox("Depatch Bounces Easy Mode", &vars::enableDepatchBouncesEasy))
				{
					if (vars::enableDepatchBounces)
					{
						vars::enableDepatchBounces = false;
						functions::sendBouncesToggleEasy();
					}
				}
				if(ksd::Checkbox("Depatch Elevators", &vars::enableDepatchElevators))
				{
					functions::sendElevatorsToggle();
				}
			}
			ksd::EndChild();
		}
		ImGui::EndGroup();
		ImGui::PopStyleVar();
	}
}
