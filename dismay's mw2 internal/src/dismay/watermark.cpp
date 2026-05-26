#include "watermark.hpp"

#include <cmath>
#include <cstdio>
#include <ctime>
#include <string>
#include <vector>

#include "../../ext/imgui/imgui.h"
#include "../framework/settings.h"
#include "../game/iw4structs.hpp"
#include "../game/iw4hooks.h"
#include "../game/offsets.hpp"
#include "functions.hpp"

namespace
{
	void drawShadowText(ImDrawList* drawList, ImVec2 pos, ImU32 color, const char* text)
	{
		drawList->AddText(ImVec2(pos.x + 1.0f, pos.y + 1.0f), IM_COL32(0, 0, 0, 255), text);
		drawList->AddText(pos, color, text);
	}

	void requestScoreboardUpdate() noexcept
	{
		static DWORD lastRequestTick = 0;
		const DWORD now = GetTickCount();
		if (now - lastRequestTick < 2000)
		{
			return;
		}
		lastRequestTick = now;
		CL_AddReliableCommand(0, "s");
	}

	bool getLocalScore(iw4::score_t& out) noexcept
	{
		const int localClient = functions::getLocalClientNum();
		const std::vector<iw4::score_t> rows = functions::getScoreboardEntries();
		for (const iw4::score_t& entry : rows)
		{
			if (entry.clientNum == localClient)
			{
				out = entry;
				return true;
			}
		}
		return false;
	}

	int updateKillStreak(int kills, int deaths, bool hasScore) noexcept
	{
		static int streak = 0;
		static int lastKills = -1;
		static int lastDeaths = -1;

		if (!hasScore)
		{
			return streak;
		}

		if (lastKills < 0)
		{
			lastKills = kills;
			lastDeaths = deaths;
			return streak;
		}

		if (deaths > lastDeaths)
		{
			streak = 0;
		}
		else if (kills > lastKills)
		{
			streak += kills - lastKills;
		}

		lastKills = kills;
		lastDeaths = deaths;
		return streak;
	}

	ImU32 ratioColor(float ratio) noexcept
	{
		if (ratio >= 3.0f)
		{
			return IM_COL32(255, 215, 0, 255);
		}
		if (ratio >= 1.0f)
		{
			return IM_COL32(80, 220, 80, 255);
		}
		if (ratio >= 0.7f)
		{
			return IM_COL32(255, 191, 0, 255);
		}
		return IM_COL32(255, 60, 60, 255);
	}
}

namespace watermark
{
	void render() noexcept
	{
		if (!features::watermark)
		{
			return;
		}

		if(!functions::isInGameNotSpectating())
		{
			return;
		}
		requestScoreboardUpdate(); // fixed? it only displays if you are in game not spectating. change if you want.

		ImDrawList* drawList = ImGui::GetBackgroundDrawList();
		if (drawList == nullptr)
		{
			return;
		}

		time_t rawTime = 0;
		time(&rawTime);
		tm timeInfo{};
		localtime_s(&timeInfo, &rawTime);
		char timeBuffer[80]{};
		strftime(timeBuffer, sizeof(timeBuffer), "%H:%M:%S", &timeInfo);

		const std::string clientName = functions::readGameString(iw4::offsets::clientName);
		const char* nameText = "Unknown";
		if (!clientName.empty())
		{
			nameText = clientName.c_str();
		}

		iw4::score_t localScore{};
		const bool hasScore = getLocalScore(localScore);

		int kills = 0;
		int assists = 0;
		int deaths = 0;
		int ping = 0;
		if (hasScore)
		{
			kills = localScore.kills;
			assists = localScore.assists;
			deaths = localScore.deaths;
			ping = localScore.ping;
		}

		const int streak = updateKillStreak(kills, deaths, hasScore);
		const int fps = static_cast<int>(ImGui::GetIO().Framerate);

		float ratio = static_cast<float>(kills);
		if (deaths > 0)
		{
			ratio = static_cast<float>(kills) / static_cast<float>(deaths);
		}

		char const* brandText = "DEGEN";
		char prefixBuffer[512]{};
		sprintf_s(
			prefixBuffer,
			sizeof(prefixBuffer),
			" | %s | Kills: %d Assists: %d Deaths: %d | Streak: %d | Ratio: ",
			nameText,
			kills,
			assists,
			deaths,
			streak);

		char ratioBuffer[16]{};
		sprintf_s(ratioBuffer, sizeof(ratioBuffer), "%.2f", ratio);

		char suffixBuffer[128]{};
		sprintf_s(
			suffixBuffer,
			sizeof(suffixBuffer),
			" | FPS: %d | Ping: %d | Time: %s | By ",
			fps,
			ping,
			timeBuffer);

		char const* dismayText = "Dismay";

		const ImVec2 brandSize = ImGui::CalcTextSize(brandText);
		const ImVec2 prefixSize = ImGui::CalcTextSize(prefixBuffer);
		const ImVec2 ratioSize = ImGui::CalcTextSize(ratioBuffer);
		const ImVec2 suffixSize = ImGui::CalcTextSize(suffixBuffer);
		const ImVec2 dismaySize = ImGui::CalcTextSize(dismayText);
		const float totalWidth = brandSize.x + prefixSize.x + ratioSize.x + suffixSize.x + dismaySize.x;
		const float padding = 5.0f;
		const float x = ImGui::GetIO().DisplaySize.x - totalWidth - 15.0f;
		const float y = 15.0f;

		const float t = static_cast<float>(ImGui::GetTime());
		const float pulse = std::sin(t * 3.0f) * 0.5f + 0.5f;
		const int dismayR = static_cast<int>(pulse * 255.0f);
		const ImU32 dismayColor = IM_COL32(dismayR, 0, 0, 255);

		drawList->AddRectFilled(
			ImVec2(x - padding, y - padding),
			ImVec2(x + totalWidth + padding, y + brandSize.y + padding),
			IM_COL32(0, 0, 0, 200),
			4.0f);

		float cursorX = x;
		drawShadowText(drawList, ImVec2(cursorX, y), IM_COL32(255, 0, 0, 255), brandText);
		cursorX += brandSize.x;

		drawShadowText(drawList, ImVec2(cursorX, y), IM_COL32(255, 255, 255, 255), prefixBuffer);
		cursorX += prefixSize.x;

		drawShadowText(drawList, ImVec2(cursorX, y), ratioColor(ratio), ratioBuffer);
		cursorX += ratioSize.x;

		drawShadowText(drawList, ImVec2(cursorX, y), IM_COL32(255, 255, 255, 255), suffixBuffer);
		cursorX += suffixSize.x;

		drawShadowText(drawList, ImVec2(cursorX, y), dismayColor, dismayText);
	}
}
