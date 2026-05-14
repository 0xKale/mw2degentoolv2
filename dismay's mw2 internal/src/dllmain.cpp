#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <thread>
#include <cstdint>
#include <string>
#include <random>
#include "hooks.hpp"

void Setup(const HMODULE instance)
{
    try
    {
        // Optional: AllocConsole();
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);

        gui::Setup();
        hooks::Setup();
        gui::FinalizeBootstrapSetup();
    }
    catch (const std::exception& error)
    {
        gui::FinalizeBootstrapSetup();
        MessageBeep(MB_ICONERROR);
        MessageBoxA(0, error.what(), "Error", MB_OK | MB_ICONEXCLAMATION);
        goto UNLOAD;
    }

    // Main Loop
    while (!GetAsyncKeyState(VK_END))
    {
        // Call the animation update here
        gui::UpdateMenuTitle();

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

UNLOAD:
    hooks::Destroy();
    gui::Destroy();
    if (stdout) fclose(stdout); // Clean up the console file handle

    FreeLibraryAndExitThread(instance, 0);
}

BOOL WINAPI DllMain(const HMODULE instance, const std::uintptr_t reason, const void* reserved)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(instance);
        const auto thread = CreateThread(nullptr, 0, reinterpret_cast<LPTHREAD_START_ROUTINE>(Setup), instance, 0, nullptr);
        if (thread) CloseHandle(thread);
    }
    return TRUE;
}