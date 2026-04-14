#include <iostream>
#include <vector>
#include "engine_hooks.h"
#include "variables.h"

Notify_fp oNotify = nullptr;
BYTE originalBytes[10];

ConvertToString_t ConvertToString = (ConvertToString_t)0x05799B0;
GetSelf_t GetSelf = (GetSelf_t)0x57B060;
SV_GameSendServerCommand_t SV_GameSendServerCommand = (SV_GameSendServerCommand_t)0x588340;
Cbuf_AddText_t Cbuf_AddText = (Cbuf_AddText_t)0x563BE0;
OpenMenu_t OpenMenu = (OpenMenu_t)0x59DDE0;
StartMatch_t Host_StartMatch = (StartMatch_t)0x4D74D0;
BalanceTeams_t BalanceTeams = (BalanceTeams_t)0x4D73B0;
MapRestart_t MapRestart = (MapRestart_t)0x5850A0;
SV_SpawnServer_t SV_SpawnServer = (SV_SpawnServer_t)0x589D90;
Cbuf_AddCall_t Cbuf_AddCall = (Cbuf_AddCall_t)0x563C90;

struct TeamPos {
    float x, y, z, viewX, viewY;
};

// Red Team Spawn Set
std::vector<TeamPos> redSpawns = {
    {3349.53f, 5061.11f, 192.125f, -90.0f, -0.137329f},
    {3196.97f, 5042.33f, 192.125f, -90.0f, -0.137329f},
    {3033.80f, 5044.31f, 192.125f, -90.0f, -0.137329f},
    {3687.09f, 4924.04f, 192.125f, -90.0f, -0.137329f},
    {3888.89f, 4598.06f, 192.125f, -90.0f, -0.137329f},
    {3968.27f, 3696.60f, 192.125f, -90.0f, -0.137329f}
};

// Blue Team Spawn Set
std::vector<TeamPos> blueSpawns = {
    {4346.41f, 3531.06f, 192.125f, -90.0f, 0.0f},
    {4388.75f, 2896.36f, 192.125f, -90.0f, 0.0f},
    {4408.88f, 2721.72f, 192.125f, -90.0f, 0.0f},
    {4408.08f, 2531.93f, 192.125f, -90.0f, 0.0f},
    {4129.40f, 2060.03f, 193.125f, -90.0f, 0.0f},
    {4048.84f, 2074.93f, 193.125f, -90.0f, 0.0f}
};

namespace engine {


    uintptr_t GetPlayerBase(int index) {
        if (index < 0 || index >= 18) return 0;
        return BASE_ADDRESS + (index * PLAYER_STRUCT_SIZE);
    }

    int GetLocalIndex() {
        return Read<int>(PLAYER_INDEX_ADDR);
    }

    int getClientTeam(int index) {
        // 0x1B1131C is the Team Base your friend uses
        const uintptr_t TEAM_BASE = 0x1B1131C;

        // Read the team directly from the parallel array
        return Read<int>(TEAM_BASE + (index * 0x366C));
    }

    void SetForce(int index, float x, float y, float z) {
        uintptr_t base = GetPlayerBase(index);
        if (base) {
            Write<float>(base + OFF_FORCE_X, x);
            Write<float>(base + OFF_FORCE_Y, y);
            Write<float>(base + OFF_FORCE_Z, z);
        }
    }

    void SetLocation(int index, float x, float y, float z, float viewx, float viewy) {
        uintptr_t base = GetPlayerBase(index);
        if (base) {
            Write<float>(base + OFF_POS_X, x);
            Write<float>(base + OFF_POS_Y, y);
            Write<float>(base + OFF_POS_Z, z);
            Write<float>(base + OFF_ANGLES_X, viewx);
            Write<float>(base + OFF_ANGLES_Y, viewy);
        }
    }

    Position GetLocation(int index) {
        uintptr_t base = GetPlayerBase(index);
        Position currentPos = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
        if (base) {
            currentPos.x = Read<float>(base + OFF_POS_X);
            currentPos.y = Read<float>(base + OFF_POS_Y);
            currentPos.z = Read<float>(base + OFF_POS_Z);
            currentPos.viewx = Read<float>(base + OFF_ANGLES_X);
            currentPos.viewy = Read<float>(base + OFF_ANGLES_Y);
        }
        return currentPos;
    }

    void* BuildTrampoline(void* target, size_t len) {
        BYTE* trampoline = (BYTE*)VirtualAlloc(NULL, len + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
        if (!trampoline) return nullptr;
        memcpy(trampoline, target, len);
        uintptr_t jmpBackAddr = ((uintptr_t)target + len) - ((uintptr_t)trampoline + len) - 5;
        trampoline[len] = 0xE9;
        *(DWORD*)(trampoline + len + 1) = (DWORD)jmpBackAddr;
        FlushInstructionCache(GetCurrentProcess(), trampoline, len + 5);
        return (void*)trampoline;
    }

    void PlaceHook(void* target, void* detour, size_t len) {
        DWORD oldProtect;
        if (!VirtualProtect(target, len, PAGE_EXECUTE_READWRITE, &oldProtect)) return;
        memset(target, 0x90, len);
        uintptr_t relativeOffset = ((uintptr_t)detour - (uintptr_t)target) - 5;
        *(BYTE*)target = 0xE9;
        *(DWORD*)((uintptr_t)target + 1) = (DWORD)relativeOffset;
        VirtualProtect(target, len, oldProtect, &oldProtect);
        FlushInstructionCache(GetCurrentProcess(), target, len);
    }

    void ResetAllClients() {
        for (int i = 0; i < 18; i++) {
            variables::bClient[i] = false;
        }
    }

    void __cdecl engine::Notify_hk(unsigned int notifyListOwnerId, unsigned int stringValue, VariableValue* top) {
        const char* NotifyId = ConvertToString(stringValue);
/*
    if (NotifyId && !strcmp(NotifyId, "player_spawned")) {
        if (variables::bMitiTerminalRemake) {
            if (notifyListOwnerId >= 0 && notifyListOwnerId < 18) {

                int team = getClientTeam(notifyListOwnerId);
                TeamPos picked;
                bool shouldTeleport = false;

                if (team == 1) { // Red Team
                    picked = redSpawns[rand() % redSpawns.size()];
                    shouldTeleport = true;
                }
                else if (team == 2) { // Blue Team
                    picked = blueSpawns[rand() % blueSpawns.size()];
                    shouldTeleport = true;
                }

                if (shouldTeleport) {
                    // Teleport ONLY the specific spawning player ID
                    SetLocation(notifyListOwnerId, picked.x, picked.y, picked.z, picked.viewX, picked.viewY);

                    if (variables::debug) {
                        std::cout << "[+] Teleported Spawning Client " << notifyListOwnerId
                            << " (Team " << team << ") to random spawn." << std::endl;
                    }
                }
            }
        }
    }
 */


        // Call original notify function to maintain game stability
        if (oNotify) {
            oNotify(notifyListOwnerId, stringValue, top);
        }
    }

    void InstallHooks() {
        void* targetAddr = (void*)0x57FA70;
        size_t len = 6;
        memcpy(originalBytes, targetAddr, len);
        oNotify = (Notify_fp)BuildTrampoline(targetAddr, len);
        PlaceHook(targetAddr, (void*)Notify_hk, len);
    }

    void RemoveHooks() {
        DWORD oldProtect;
        void* targetAddr = (void*)0x57FA70;
        size_t len = 6;
        if (VirtualProtect(targetAddr, len, PAGE_EXECUTE_READWRITE, &oldProtect)) {
            memcpy(targetAddr, originalBytes, len);
            VirtualProtect(targetAddr, len, oldProtect, &oldProtect);
            FlushInstructionCache(GetCurrentProcess(), targetAddr, len);
        }
    }

}
