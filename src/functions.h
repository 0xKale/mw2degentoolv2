#pragma once
#include "../ext/imgui/imgui.h"
#include "../src/gui.h"
#include <cstdint>

typedef void(__cdecl* SV_GameSendServerCommand_t)(int clientNum, int reliable, char* command);
typedef void(__cdecl* Cbuf_AddText_t)(int localClientNum, const char* text);
typedef void(__cdecl* OpenMenu_t)(int localClientNum, const char* menuName);
typedef void(__cdecl* SV_SpawnServer_t)(char* name, int, int);
typedef void(__cdecl* BalanceTeams_t)(void* PartyData_s_party);
typedef void(__cdecl* MapRestart_t)(int fastRestart, int unknown);
typedef void(__cdecl* Cbuf_AddCall_t)(int a1, void* a2);

extern SV_GameSendServerCommand_t SV_GameSendServerCommand;
extern Cbuf_AddText_t Cbuf_AddText;
extern OpenMenu_t OpenMenu;
extern SV_SpawnServer_t SV_SpawnServer;
extern BalanceTeams_t BalanceTeams;
extern MapRestart_t MapRestart;
extern Cbuf_AddCall_t Cbuf_AddCall;

constexpr uintptr_t G_LOBBYDATA = 0x10F91E8;
constexpr uintptr_t PARTYSESSION_P = 0x10F5A18;

// using namespace gui;

namespace functions
{
	void handleMouseCursor();
	void sendFPSandFOV();
	void sendMapSize();
	int getHostId();
	int GetAmmo(int client);
	void funChat();
	void funFOVMin();
	void funMouseFix();
	void pitchspeed();
	void yawspeed();
	void mouseAccel();
	void bypassMouseInput();
	void debug();
	void mousefilter();
	void sendNoCamo();
	void sendNoFog();
	void sendNoBullets();
	void fuckTheSunAway();
	void sendMovie();
	void clearGlass();

	void doSaveBarracks();
	void doGiveDeag();
	void writeMemory(DWORD dwAddress, void *bytes, DWORD dwSize);
	void unlockAll();
	void doLevel70();
	void doLevel1();
	void sendPrestige(int prestige);
	void doDLCMaps();

	void doMaxPlayers(int amount);
	void doStartMatch();
	void doBalanceTeams();
	void FastRestart();
	void ChangeMap();
	void ChangeGamemode();
	void doForceHost();
	void doFFATeamFix();

	void handleHotkeys();
	void sendViewModel();
	void sendOpenMenu(const char *menu);

	void sendBouncesToggle();
	void sendElevatorsToggle();
	char *getPlayerName(int client);
	void sendCustomPort();
	void menuUITweaks();
	void writeSensitivity(float sens);
	float readSensitivity();
	void doIronSight();

}
