#include "config.h"
#include "variables.h"
#include "functions.h"
#include <shlobj.h>  // For getting folder paths if needed
#include <windows.h> // For GetModuleHandleExA, HMODULE, MAX_PATH
#include <iostream>

std::string config::GetConfigPath()
{
    char path[MAX_PATH];
    HMODULE hm = NULL;

    // Get the handle to the DLL itself using the address of this function
    if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                               GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCSTR)&GetConfigPath, &hm) == 0)
    {
        return "dismay_config.ini"; // Fallback
    }

    // Get the full path to the DLL file
    if (GetModuleFileNameA(hm, path, sizeof(path)) == 0)
    {
        return "dismay_config.ini"; // Fallback
    }

    std::string strPath = path;
    // Remove the dll filename to get the directory
    size_t lastSlash = strPath.find_last_of("\\/");
    if (lastSlash != std::string::npos)
    {
        strPath = strPath.substr(0, lastSlash);
    }
    return strPath + "\\dismay_config.ini";
}
void config::Save()
{
    std::string file = GetConfigPath();

    // Helper lambda to save different types
    auto WriteInt = [&](const char *section, const char *key, int val)
    {
        WritePrivateProfileStringA(section, key, std::to_string(val).c_str(), file.c_str());
    };
    auto WriteFloat = [&](const char *section, const char *key, float val)
    {
        WritePrivateProfileStringA(section, key, std::to_string(val).c_str(), file.c_str());
    };
    auto WriteBool = [&](const char *section, const char *key, bool val)
    {
        WritePrivateProfileStringA(section, key, val ? "1" : "0", file.c_str());
    };

    // --- Save Variables ---
    WriteInt("Visuals", "FPS", variables::iFPS);
    WriteFloat("Visuals", "FOV", variables::fFieldOfView);
    WriteBool("Visuals", "DrawSun", variables::bDrawSun);
    WriteBool("Visuals", "NoCamoEnabled", variables::fNoCamoBool);
    WriteBool("Visuals", "NoFogEnabled", variables::fNoCFogBool);
    WriteBool("Visuals", "NoBulletsEnabled", variables::fNoBulletsBool);
    WriteBool("Visuals", "MovieMode", variables::fMovieBool);
    WriteBool("Visuals", "ClearGlass", variables::bGlass);
    WriteInt("Visuals", "Fullbright", variables::iFullbright);
    WriteInt("Visuals", "LightMap", variables::iLightMap);
    WriteInt("Visuals", "NoCamoValue1", variables::fNoCamo1);
    WriteInt("Visuals", "NoCamoValue2", variables::fNoCamo2);
    WriteInt("Visuals", "NoFogValue1", variables::fNoFog1);
    WriteInt("Visuals", "NoFogValue2", variables::fNoFog2);
    WriteInt("Visuals", "NoBulletsValue", variables::fNoBullets);
    WriteInt("Visuals", "MovieValue", variables::fMovie);

    WriteBool("Misc", "Chat", variables::bChat);
    WriteBool("Misc", "Mouse11", variables::fFOVMin);
    WriteFloat("Misc", "Mouse11Value", variables::fFOVMinSlider);
    WriteBool("Misc", "MouseFix", variables::bMouseFix);
    WriteFloat("Misc", "MapSize", variables::fMapSize);
    WriteBool("Misc", "UseInsertKey", variables::bUseInsertKey);
    WriteBool("Misc", "PingText", variables::bPingText);

    // Flush INI cache to disk
    WritePrivateProfileStringA(NULL, NULL, NULL, file.c_str());
    std::cout << "[Config] Saved to: " << file << std::endl;
}

void config::Load()
{
    std::string file = GetConfigPath();

    // Helper lambda to load different types
    auto ReadInt = [&](const char *section, const char *key, int def) -> int
    {
        return GetPrivateProfileIntA(section, key, def, file.c_str());
    };
    auto ReadFloat = [&](const char *section, const char *key, float def) -> float
    {
        char result[32];
        GetPrivateProfileStringA(section, key, std::to_string(def).c_str(), result, 32, file.c_str());
        return (float)atof(result);
    };
    auto ReadBool = [&](const char *section, const char *key, bool def) -> bool
    {
        return GetPrivateProfileIntA(section, key, def ? 1 : 0, file.c_str()) == 1;
    };

    // --- Load Variables ---
    variables::iFPS = ReadInt("Visuals", "FPS", 400);
    variables::fFieldOfView = ReadFloat("Visuals", "FOV", 90.0f);
    variables::bDrawSun = ReadBool("Visuals", "DrawSun", true);
    variables::fNoCamoBool = ReadBool("Visuals", "NoCamoEnabled", false);
    variables::fNoCFogBool = ReadBool("Visuals", "NoFogEnabled", false);
    variables::fNoBulletsBool = ReadBool("Visuals", "NoBulletsEnabled", false);
    variables::fMovieBool = ReadBool("Visuals", "MovieMode", false);
    variables::iFullbright = ReadInt("Visuals", "Fullbright", 1);
    variables::iLightMap = ReadInt("Visuals", "LightMap", 0);
    variables::fNoCamo1 = ReadInt("Visuals", "NoCamoValue1", 0);
    variables::fNoCamo2 = ReadInt("Visuals", "NoCamoValue2", 0);
    variables::fNoFog1 = ReadInt("Visuals", "NoFogValue1", 0);
    variables::fNoFog2 = ReadInt("Visuals", "NoFogValue2", 0);
    variables::fNoBullets = ReadInt("Visuals", "NoBulletsValue", 0);
    variables::fMovie = ReadInt("Visuals", "MovieValue", 1);

    variables::bChat = ReadBool("Misc", "Chat", true);
    variables::fFOVMin = ReadBool("Misc", "Mouse11", false);
    variables::fFOVMinSlider = ReadFloat("Misc", "Mouse11Value", 90.0f);
    variables::bMouseFix = ReadBool("Misc", "MouseFix", true);
    variables::fMapSize = ReadFloat("Misc", "MapSize", 1.0f);
    variables::bUseInsertKey = ReadBool("Misc", "UseInsertKey", true);
    variables::bGlass = ReadBool("Visuals", "ClearGlass", false);
    variables::bPingText = ReadBool("Misc", "PingText", true);
    Cbuf_AddText(0, variables::bPingText ? "cg_scoreboardPingText 1" : "cg_scoreboardPingText 0");


    /*
        std::cout << "[Config] Loaded from: " << file << std::endl;
    std::cout << "[Config] FPS=" << variables::iFPS
              << " FOV=" << variables::fFieldOfView
              << " Fullbright=" << variables::iFullbright
              << " LightMap=" << variables::iLightMap << std::endl;
    */
}
