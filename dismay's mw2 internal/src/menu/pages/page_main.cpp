#include "pages.hpp"

#include "../../../ext/imgui/imgui.h"
#include "../../../ext/imgui/imgui_internal.h"
#include "../../../ext/fonts/iconsfontawesome/IconsFontAwesome6.h"

#include "../../framework/framework.hpp"
#include "../../dismay/functions.hpp"
#include "../../dismay/crosshair_sharecode.h"
#include "../../game/iw4hooks.h"

#include <string>

namespace vars {

	bool enableTextChat = true;
	bool enableMouseOneToOne = false;
	bool ironSightIntervention = false;

	bool noSun = true;
	bool drawCamo = true;
	bool drawFog = true;
	bool drawBullets = true;
	bool movieMode = false;
	bool clearGlass = false;
	bool pingText = true;

	float mouseSensitivity = functions::readSensitivity();
	float defaultFovMin = 1.0f;

	int framesPerSecond = 400;
	float fieldOfView = 90.0f;
	float mapSize = 1.000f;



	int selectedFullbrightMode = 1;

	const char* fullbrightModes[5]{
		"Invert",
		"Normal",
		"Super",
		"Slight",
		"Dullish",
	};

	char console[256] = "";

	bool enableDLC = false;
	bool enableCustomPort = false;
	int customPort = 28961;

	int fullbright = 0;
	int lightmap = 1;

	bool enableCrosshair = false;
	ImVec4 crosshair_color = ImColor(225, 255, 255);
	bool crosshairOutline = true;
	float crosshairGap = 1.8f;
	float crosshairLength = 3.5f;
	float crosshairThickness = 1.3f;
	float crosshairOutlineThickness = 1.0f;
	float crosshairScale = 1.0f;
	float crosshairLengthScale = 1.0f;
	float crosshairGapScale = 1.0f;
	bool crosshairCenterDot = false;
	bool crosshairTStyle = false;
	char csShareCodeInput[48] = "";

	float fcg_gun_x = 0.0f;
	float fcg_gun_y = 0.0f;
	float fcg_gun_z = 0.0f;

}

namespace {
	bool FullbrightPresetActive(const int fb, const int lm) noexcept
	{
		return vars::fullbright == fb && vars::lightmap == lm;
	}

	// Must match ksd::BeginChild padding in child.cpp
	constexpr float childContentPadTop = 50.f;
	constexpr float panelGap = 10.f;
}

namespace menu_pages {
	void RenderMainPage() noexcept {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 3.f));
		ImGui::BeginGroup();
		{
			const ImVec2 origin(ImGui::GetCursorPos());
			const float leftX = origin.x - layout::leftPull;
			const float rightX = origin.x + layout::colWidth + layout::colGap;
			const float mainContentHeight = vars::enableCustomPort ? 575.f : 535.f;
			const float crosshairContentHeight = 230.f;
			const float mainOuterHeight = mainContentHeight + childContentPadTop;
			const float crosshairY = origin.y + mainOuterHeight + panelGap;

			ImGui::SetCursorPos(ImVec2(leftX, origin.y));
			if(ksd::BeginChild(ICON_FA_CROSSHAIRS, "Main", mainContentHeight, layout::leftWidth))
			{
				if(ksd::Checkbox("Enable Text Chat", &vars::enableTextChat))
				{
					functions::toggleChat();
					if(vars::enableTextChat)
					{
						SendNotify("Chat Enabled", 2.0f);
					}
					else
					{
						SendNotify("Chat Disabled", 2.0f);
					}
				}
				if(ksd::Checkbox("Enable Mouse 1:1", &vars::enableMouseOneToOne))
				{
					if(vars::enableMouseOneToOne)
					{
						SendNotify("Mouse 1:1 Enabled", 2.0f);
					}
					else
					{
						SendNotify("Mouse 1:1 Disabled", 2.0f);
						//functions::sendFOVMin();
					}

				}
				if(ksd::Checkbox("Iron Sight Intervention", &vars::ironSightIntervention))
				{
					functions::doIronSight();
					if(vars::ironSightIntervention)
					{
						SendNotify("Iron Sight Intervention Enabled", 2.0f);
					}
					else
					{
						SendNotify("Iron Sight Intervention Disabled", 2.0f);
					}

				}
				ksd::InputFloat("Sensitivity", &vars::mouseSensitivity);
				if(ksd::Button("Send Sensitivity", ImVec2(280.f, 30.f)))
				{
					functions::writeSensitivity(vars::mouseSensitivity);
					if(vars::mouseSensitivity)
					{
						SendNotify("Sensitivity Sent", 2.0f);
					}
				}
				ksd::SliderInt("Frames Per Second", &vars::framesPerSecond, 30, 1000);
				ksd::SliderFloat("Field Of View", &vars::fieldOfView, 65.0f, 120.0f, "%.0f");
				ksd::SliderFloat("Map Size", &vars::mapSize, 1.f, 2.0f);
				ImGui::Dummy(ImVec2(0.f, 10.f));
				if(ksd::Button("Disconnect", ImVec2(280.f, 30.f)))
				{
					Cbuf_AddText(0, "disconnect");
					SendNotify("Disconnected", 2.0f);
				}
				ksd::Text("Console");
				ImGui::Dummy(ImVec2(0.f, 1.f));
				ksd::InputTextOnly("##console", vars::console, sizeof(vars::console), 290.f);
				if(ksd::Button("Send Console", ImVec2(280.f, 30.f)))
				{
					Cbuf_AddText(0, vars::console);
					SendNotify("Sent To Console", 2.0f);

				}
				if(ksd::Checkbox("Enable DLC?", &vars::enableDLC))
                {

                    if(vars::enableDLC)
                    {
                        SendNotify("DLC Enabled", 2.0f);
                    }
                    else
                    {
                        SendNotify("DLC Disabled", 2.0f);
                    }
                }
				if(ksd::Checkbox("Enable Custom Port?", &vars::enableCustomPort))
				{
					functions::sendCustomPort();
					if(vars::enableCustomPort)
					{
						SendNotify("Custom Port Enabled", 2.0f);
					}
					else
					{
						SendNotify("Custom Port Disabled", 2.0f);
					}
				}
				if (vars::enableCustomPort)
				{
					ksd::InputInt("Custom Port", &vars::customPort);
				}
				if(ksd::Button("Force Team Change", ImVec2(280.f, 30.f)))
				{
					functions::forceTeamChange();
					SendNotify("Team Changed", 2.0f);
				}
				if(ksd::Checkbox("Watermark", &features::watermark))
				{
					if(features::watermark)
					{
						SendNotify("Watermark Enabled", 2.0f);
					}
					else
					{
						SendNotify("Watermark Disabled", 2.0f);
					}
				}
			}
			ksd::EndChild();

			ImGui::SetCursorPos(ImVec2(leftX, crosshairY));
			if(ksd::BeginChild(ICON_FA_CROSSHAIRS, "Crosshair", crosshairContentHeight, layout::leftWidth))
			{
				if(ksd::Checkbox("Enable Crosshair", &vars::enableCrosshair))
				{
					functions::fuckTheCrosshairAway();
					if(vars::enableCrosshair)
					{
						SendNotify("Crosshair Enabled", 2.0f);
					}
					else
					{
						SendNotify("Crosshair Disabled", 2.0f);
					}
				}
				ImGui::ColorEdit4("Crosshair Color", (float*)&vars::crosshair_color, ImGuiColorEditFlags_AlphaPreview | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_AlphaBar | ImGuiColorEditFlags_NoInputs);
				ksd::Checkbox("Crosshair Outline", &vars::crosshairOutline);
				ksd::Checkbox("Center Dot", &vars::crosshairCenterDot);
				ksd::Checkbox("T-Style", &vars::crosshairTStyle);
				ksd::SliderFloat("Crosshair Scale", &vars::crosshairScale, 0.5f, 2.0f, "%.2f");
				ksd::SliderFloat("Crosshair Length", &vars::crosshairLengthScale, 0.5f, 3.0f, "%.2f");
				ksd::SliderFloat("Crosshair Gap", &vars::crosshairGapScale, 0.5f, 3.0f, "%.2f");
/*
    ImGui::Dummy(ImVec2(0.f, 4.f));
				ksd::Text("CS2 Crosshair Code");
				ImGui::Dummy(ImVec2(0.f, 1.f));
				ksd::InputTextOnly("##csCrosshair", vars::csShareCodeInput, sizeof(vars::csShareCodeInput), 290.f);
				if (ksd::Button("Apply CS Crosshair", ImVec2(280.f, 30.f)))
				{
					if (ApplyCsCrosshairToVars(vars::csShareCodeInput))
						SendNotify("Crosshair Imported", 2.0f);
					else
						SendNotify("Invalid Share Code", 2.0f);
				}
 */
			}
			ksd::EndChild();

			ImGui::SetCursorPos(ImVec2(rightX, origin.y));
			if(ksd::BeginChild(ICON_FA_KEYBOARD, "Toggles", 230.f, layout::rightWidth))
			{
				if(ksd::Checkbox("Draw Sun", &vars::noSun))
				{
					functions::fuckTheSunAway();
					if(vars::noSun)
						SendNotify("Draw Sun Enabled", 2.0f);
					else
						SendNotify("Draw Sun Disabled", 2.0f);
				}
				if(ksd::Checkbox("Draw Camos", &vars::drawCamo))
				{
					functions::sendNoCamo();
					if(vars::drawCamo)
						SendNotify("Draw Camos Enabled", 2.0f);
					else
						SendNotify("Draw Camos Disabled", 2.0f);
				}
				if(ksd::Checkbox("Draw Fog", &vars::drawFog))
				{
					functions::sendNoFog();
					if(vars::drawFog)
						SendNotify("Draw Fog Enabled", 2.0f);
					else
						SendNotify("Draw Fog Disabled", 2.0f);
				}
				if(ksd::Checkbox("Draw Bullets", &vars::drawBullets))
				{
					functions::sendNoBullets();
					if(vars::drawBullets)
						SendNotify("Draw Bullets Enabled", 2.0f);
					else
						SendNotify("Draw Bullets Disabled", 2.0f);
				}
				if(ksd::Checkbox("Movie Mode", &vars::movieMode))
				{
					functions::sendMovie();
					if(vars::movieMode)
						SendNotify("Movie Mode Enabled", 2.0f);
					else
						SendNotify("Movie Mode Disabled", 2.0f);
				}
				if(ksd::Checkbox("Clear Glass", &vars::clearGlass))
				{
					functions::clearGlass();
					if(vars::clearGlass)
						SendNotify("Clear Glass Enabled", 2.0f);
					else
						SendNotify("Clear Glass Disabled", 2.0f);
				}
				if(ksd::Checkbox("Ping Text", &vars::pingText))
				{
					functions::sendPingText();
					if(vars::pingText)
						SendNotify("Ping Text Enabled", 2.0f);
					else
						SendNotify("Ping Text Disabled", 2.0f);
				}

			}
			ksd::EndChild();


			ImGui::SetCursorPos(ImVec2(rightX, origin.y + 290.f));
			if(ksd::BeginChild(ICON_FA_GEAR, "Fullbright", 190.f, layout::rightWidth))
			{
				if(ksd::Button("Invert", ImVec2(280.f, 30.f), FullbrightPresetActive(0, 0)))
				{
					vars::selectedFullbrightMode = 0;
					vars::fullbright = 0;
					vars::lightmap = 0;
					Cbuf_AddText(0, "r_fullbright 0;r_lightMap 0;");
					SendNotify("Inverted Fullbright", 2.0f);
				}
				if(ksd::Button("Normal", ImVec2(280.f, 30.f), FullbrightPresetActive(0, 1)))
				{
					vars::selectedFullbrightMode = 1;
					vars::fullbright = 0;
					vars::lightmap = 1;
					Cbuf_AddText(0, "r_fullbright 0;r_lightMap 1;");
					SendNotify("Fullbright Off", 2.0f);
				}
				if(ksd::Button("Super", ImVec2(280.f, 30.f), FullbrightPresetActive(0, 2)))
				{
					vars::selectedFullbrightMode = 2;
					vars::fullbright = 0;
					vars::lightmap = 2;
					Cbuf_AddText(0, "r_fullbright 0;r_lightMap 2;");
					SendNotify("Super Fullbright", 2.0f);
				}
				if(ksd::Button("Slight", ImVec2(280.f, 30.f), FullbrightPresetActive(0, 3)))
				{
					vars::selectedFullbrightMode = 3;
					vars::fullbright = 0;
					vars::lightmap = 3;
					Cbuf_AddText(0, "r_fullbright 0;r_lightMap 3;");
					SendNotify("Slight Fullbright", 2.0f);
				}
				if(ksd::Button("Dullish", ImVec2(280.f, 30.f), FullbrightPresetActive(1, 0)))
				{
					vars::selectedFullbrightMode = 4;
					vars::fullbright = 1;
					vars::lightmap = 0;
					Cbuf_AddText(0, "r_fullbright 1;r_lightMap 1;");
					SendNotify("Dullish Fullbright", 2.0f);
				}
			}
			ksd::EndChild();

			ImGui::SetCursorPos(ImVec2(rightX, origin.y + 540.f));
			if(ksd::BeginChild(ICON_FA_GUN, "View Model", 155.f, layout::rightWidth))
			{
				if(ksd::SliderFloat("Gun X", &vars::fcg_gun_x, -10.f, 10.f, "%.2f"))
					functions::sendViewModel();
				if(ksd::SliderFloat("Gun Y", &vars::fcg_gun_y, -10.f, 10.f, "%.2f"))
					functions::sendViewModel();
				if(ksd::SliderFloat("Gun Z", &vars::fcg_gun_z, -10.f, 10.f, "%.2f"))
					functions::sendViewModel();
				if(ksd::Button("Reset View Model", ImVec2(280.f, 30.f)))
				{
				// Default values
					vars::fcg_gun_x = 0.0f;
					vars::fcg_gun_y = 0.0f;
					vars::fcg_gun_z = 0.0f;
					functions::sendViewModel();
					SendNotify("View Model Reset", 2.0f);
				}
			}
			ksd::EndChild();
		}
		ImGui::EndGroup();
		ImGui::PopStyleVar();
	}
}
