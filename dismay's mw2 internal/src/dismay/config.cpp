#include "config.h"
#include "functions.hpp"
#include "../menu/pages/page_main.hpp"
#include "../framework/settings.h"
#include "../game/iw4hooks.h"

#include <Windows.h>
#include <string>
#include <cstdlib>

std::string config::GetConfigPath()
{
	char path[MAX_PATH];
	HMODULE hm = NULL;

	if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
							GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
						   (LPCSTR)&GetConfigPath, &hm) == 0)
	{
		return "dismay_config.ini";
	}

	if (GetModuleFileNameA(hm, path, sizeof(path)) == 0)
	{
		return "dismay_config.ini";
	}

	std::string strPath = path;
	size_t lastSlash = strPath.find_last_of("\\/");
	if (lastSlash != std::string::npos)
		strPath = strPath.substr(0, lastSlash);

	return strPath + "\\dismay_config.ini";
}

void config::Save()
{
	std::string file = GetConfigPath();

	auto WriteInt = [&](const char* section, const char* key, int val) {
		WritePrivateProfileStringA(section, key, std::to_string(val).c_str(), file.c_str());
	};
	auto WriteFloat = [&](const char* section, const char* key, float val) {
		WritePrivateProfileStringA(section, key, std::to_string(val).c_str(), file.c_str());
	};
	auto WriteBool = [&](const char* section, const char* key, bool val) {
		WritePrivateProfileStringA(section, key, val ? "1" : "0", file.c_str());
	};

	WriteInt("Visuals", "FPS", vars::framesPerSecond);
	WriteFloat("Visuals", "FOV", vars::fieldOfView);
	WriteBool("Visuals", "DrawSun", vars::noSun);
	WriteBool("Visuals", "NoCamoEnabled", vars::noCamo);
	WriteBool("Visuals", "NoFogEnabled", vars::noFog);
	WriteBool("Visuals", "NoBulletsEnabled", vars::noBullets);
	WriteBool("Visuals", "MovieMode", vars::movieMode);
	WriteBool("Visuals", "ClearGlass", vars::clearGlass);
	WriteInt("Visuals", "Fullbright", vars::fullbright);
	WriteInt("Visuals", "LightMap", vars::lightmap);

	WriteBool("Misc", "Chat", vars::enableTextChat);
	WriteBool("Misc", "Mouse11", vars::enableMouseOneToOne);
	WriteBool("Misc", "MouseFix", vars::mouseFix);
	WriteFloat("Misc", "MapSize", vars::mapSize);
	WriteBool("Misc", "PingText", vars::pingText);

	WriteInt("UI", "AccentR", static_cast<int>(colors::accent_color.x * 255.f));
	WriteInt("UI", "AccentG", static_cast<int>(colors::accent_color.y * 255.f));
	WriteInt("UI", "AccentB", static_cast<int>(colors::accent_color.z * 255.f));
	WriteInt("UI", "AccentA", static_cast<int>(colors::accent_color.w * 255.f));

	WriteBool("UI", "EnableCrosshair", vars::enableCrosshair);
	WriteBool("UI", "CrosshairOutline", vars::crosshairOutline);
	WriteInt("UI", "CrosshairR", static_cast<int>(vars::crosshair_color.x * 255.f));
	WriteInt("UI", "CrosshairG", static_cast<int>(vars::crosshair_color.y * 255.f));
	WriteInt("UI", "CrosshairB", static_cast<int>(vars::crosshair_color.z * 255.f));
	WriteInt("UI", "CrosshairA", static_cast<int>(vars::crosshair_color.w * 255.f));

	WritePrivateProfileStringA(NULL, NULL, NULL, file.c_str());
}

void config::Load()
{
	std::string file = GetConfigPath();

	if (GetFileAttributesA(file.c_str()) == INVALID_FILE_ATTRIBUTES)
		return;

	auto ReadInt = [&](const char* section, const char* key, int def) -> int {
		return GetPrivateProfileIntA(section, key, def, file.c_str());
	};
	auto ReadFloat = [&](const char* section, const char* key, float def) -> float {
		char result[32];
		GetPrivateProfileStringA(section, key, std::to_string(def).c_str(), result, 32, file.c_str());
		return static_cast<float>(atof(result));
	};
	auto ReadBool = [&](const char* section, const char* key, bool def) -> bool {
		return GetPrivateProfileIntA(section, key, def ? 1 : 0, file.c_str()) == 1;
	};

	vars::framesPerSecond   = ReadInt("Visuals", "FPS", 400);
	vars::fieldOfView       = ReadFloat("Visuals", "FOV", 90.0f);
	vars::noSun             = ReadBool("Visuals", "DrawSun", true);
	vars::noCamo            = ReadBool("Visuals", "NoCamoEnabled", false);
	vars::noFog             = ReadBool("Visuals", "NoFogEnabled", false);
	vars::noBullets         = ReadBool("Visuals", "NoBulletsEnabled", false);
	vars::movieMode         = ReadBool("Visuals", "MovieMode", false);
	vars::clearGlass        = ReadBool("Visuals", "ClearGlass", false);
	vars::fullbright        = ReadInt("Visuals", "Fullbright", 0);
	vars::lightmap          = ReadInt("Visuals", "LightMap", 0);

	vars::enableTextChat    = ReadBool("Misc", "Chat", true);
	vars::enableMouseOneToOne = ReadBool("Misc", "Mouse11", false);
	vars::mouseFix          = ReadBool("Misc", "MouseFix", false);
	vars::mapSize           = ReadFloat("Misc", "MapSize", 1.0f);
	vars::pingText          = ReadBool("Misc", "PingText", true);

	int r = ReadInt("UI", "AccentR", 255);
	int g = ReadInt("UI", "AccentG", 255);
	int b = ReadInt("UI", "AccentB", 255);
	int a = ReadInt("UI", "AccentA", 255);
	colors::accent_color = ImVec4(r / 255.f, g / 255.f, b / 255.f, a / 255.f);

	vars::enableCrosshair = ReadBool("UI", "EnableCrosshair", false);
	vars::crosshairOutline = ReadBool("UI", "CrosshairOutline", true);
	int cr = ReadInt("UI", "CrosshairR", 225);
	int cg = ReadInt("UI", "CrosshairG", 255);
	int cb = ReadInt("UI", "CrosshairB", 255);
	int ca = ReadInt("UI", "CrosshairA", 255);
	vars::crosshair_color = ImVec4(cr / 255.f, cg / 255.f, cb / 255.f, ca / 255.f);

	ApplyToGame();
}

void config::ApplyToGame()
{
	functions::sendFPSandFOV();
	functions::sendMapSize();
	functions::toggleChat();
	functions::sendFOVMin();
	functions::mouseFix();
	functions::fuckTheSunAway();
	functions::sendNoCamo();
	functions::sendNoFog();
	functions::sendNoBullets();
	functions::sendMovie();
	functions::clearGlass();
	functions::sendPingText();

	const std::string lightMapCmd = "r_lightMap " + std::to_string(vars::lightmap) + ";";
	const std::string fullbrightCmd = "r_fullbright " + std::to_string(vars::fullbright) + ";";
	if (vars::fullbright != 0)
	{
		Cbuf_AddText(0, lightMapCmd.c_str());
		Cbuf_AddText(0, fullbrightCmd.c_str());
	}
	else
	{
		Cbuf_AddText(0, fullbrightCmd.c_str());
		Cbuf_AddText(0, lightMapCmd.c_str());
	}
}
