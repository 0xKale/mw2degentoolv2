#pragma once
#include <Windows.h>
#include <string>
#include <vector>

namespace config
{
    // Get the path to the config file (e.g., C:\MW2_Config.ini)
    std::string GetConfigPath();

    // Save current variables to the file
    void Save();

    // Load variables from the file
    void Load();
}