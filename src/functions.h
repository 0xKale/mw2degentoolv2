#pragma once
#include "../ext/imgui/imgui.h"
#include "../src/gui.h"

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

	// --- 8000HZ RAW INPUT FIX ---
	void InitializeRawInput(HWND hwnd);
	bool HandleRawInputMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void ApplyRawInputToCamera();

}
