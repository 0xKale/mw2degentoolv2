#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>
#include <cstdint>
#include "gui.h"
#include "hooks.h"
#include "variables.h"
#include "functions.h"
#include "config.h"
#include "dedigamer.h"
#include <iostream>
#include <string>

void AllocateConsole()
{
    AllocConsole();
    FILE *f;
    freopen_s(&f, "CONOUT$", "w", stdout);
    freopen_s(&f, "CONOUT$", "w", stderr);
    freopen_s(&f, "CONIN$", "r", stdin);
    SetConsoleTitleA("MW2 Debug Console");
}

void Setup(const HMODULE instance)
{
    AllocateConsole();
    std::cout << "[>>>] MW2 Degen Tool Loaded" << std::endl;

    // Read UseInsertKey from config before waiting
    std::string cfgPath = config::GetConfigPath();
    char buf[8] = {};
    GetPrivateProfileStringA("Misc", "UseInsertKey", "1", buf, sizeof(buf), cfgPath.c_str());
    bool useInsert = (atoi(buf) != 0);
    int activateKey = useInsert ? VK_INSERT : VK_HOME;
    const char *keyName = useInsert ? "INSERT" : "HOME";

    std::cout << "[>>>] Press " << keyName << " to activate..." << std::endl;

    while (!(GetAsyncKeyState(activateKey) & 1))
    {
        if (GetAsyncKeyState(VK_END))
            goto UNLOAD;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    std::cout << "[>>>] Activated! Initializing..." << std::endl;

    try
    {
        gui::Setup();
        hooks::Setup();
        config::Load();
        functions::sendFPSandFOV();
        functions::sendMapSize();
        functions::funChat();
        functions::funFOVMin();
        functions::funMouseFix();
        functions::fuckTheSunAway();
        functions::sendNoCamo();
        functions::sendNoFog();
        functions::sendNoBullets();
        functions::sendMovie();
        const std::string lightMapCmd = "r_lightMap " + std::to_string(variables::iLightMap) + ";";
        const std::string fullbrightCmd = "r_fullbright " + std::to_string(variables::iFullbright) + ";";
        if (variables::iFullbright != 0)
        {
            Cbuf_AddText(0, lightMapCmd.c_str());
            Cbuf_AddText(0, fullbrightCmd.c_str());
        }
        else
        {
            Cbuf_AddText(0, fullbrightCmd.c_str());
            Cbuf_AddText(0, lightMapCmd.c_str());
        }
        dedigamer::Init();
        std::cout << "[+] GUI and Hooks Initialized" << std::endl;
    }
    catch (const std::exception &error)
    {
        std::cout << "[!] Initialization Error: " << error.what() << std::endl;
        MessageBox(0, error.what(), "Error", MB_OK | MB_ICONEXCLAMATION);
        goto UNLOAD;
    }

    static DWORD lastUpdateNames = 0;

    while (!GetAsyncKeyState(VK_END))
    {

        // --- 1. HOTKEYS (Always run, open or closed) ---
        functions::handleHotkeys();

        // --- 2. GAME STATE SYNC (Stable updates) ---
        functions::sendFPSandFOV();
        functions::doFFATeamFix();
        functions::menuUITweaks(); // Updates menu text

        // --- 3. NAME UPDATES (Slow timer to stop lag) ---
        if (GetTickCount() - lastUpdateNames > 1000)
        {
            for (int i = 0; i < 18; i++)
            {
                char *namePtr = functions::getPlayerName(i);
                if (namePtr && *namePtr != '\0')
                {
                    memcpy(variables::clientName[i], namePtr, 14);
                    variables::clientName[i][14] = '\0';
                }
            }
            lastUpdateNames = GetTickCount();
        }

        // Sleep 10ms to prevent 100% CPU usage lag
        std::this_thread::sleep_for(std::chrono::milliseconds(33));
    }

UNLOAD:
    std::cout << "[!] Unloading Tool..." << std::endl;
    dedigamer::Shutdown();
    hooks::Destroy();
    gui::Destroy();
    FreeLibraryAndExitThread(instance, 0);
}

BOOL WINAPI DllMain(const HMODULE instance, const std::uintptr_t reason, const void *reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        const auto thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Setup), instance, 0, nullptr);
        if (thread)
            CloseHandle(thread);
    }
    return TRUE;
}
