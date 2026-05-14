#pragma once

#include <cstdint>

typedef void(__cdecl* SV_GameSendServerCommand_t)(int clientNum, int reliable, char* command);
extern SV_GameSendServerCommand_t SV_GameSendServerCommand;

typedef void(__cdecl* Cbuf_AddText_t)(int localClientNum, const char* text);
extern Cbuf_AddText_t Cbuf_AddText;

typedef void(__cdecl* Cbuf_AddCall_t)(int a1, void* a2);
extern Cbuf_AddCall_t Cbuf_AddCall;

typedef void(__cdecl* OpenMenu_t)(int localClientNum, const char* menuName);
extern OpenMenu_t OpenMenu;

typedef void(__cdecl* SV_SpawnServer_t)(char* name, int, int);
extern SV_SpawnServer_t SV_SpawnServer;

typedef void(__cdecl* BalanceTeams_t)(void* PartyData_s_party);
extern BalanceTeams_t BalanceTeams;

typedef void(__cdecl* MapRestart_t)(int fastRestart, int unknown);
extern MapRestart_t MapRestart;
