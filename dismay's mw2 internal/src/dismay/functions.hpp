#pragma once

#include <cstdint>
#include <string>
#include <vector>
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "../game/iw4structs.hpp"

namespace functions
{
	void startFeatureWorker() noexcept;
	void stopFeatureWorker() noexcept;
	void syncImGuiMouseDrawCursor() noexcept;
	void handleMouseCursor();

	void SetDvarInt(std::uintptr_t dvarAddress, int value);
	void SetDvarFloat(std::uintptr_t dvarAddress, float value);
	int getHostId() noexcept;

	void fuckTheSunAway() noexcept;
	void clearGlass() noexcept;
	void sendNoCamo() noexcept;
	void sendNoFog() noexcept;
	void sendNoBullets() noexcept;
	void sendMovie() noexcept;
	void sendFPSandFOV() noexcept;
	void sendMapSize() noexcept;
	void sendFOVMin() noexcept;
	void toggleChat() noexcept;
	void mouseFix();
	void doSaveBarracks();
	void loadProfileStats() noexcept;
	void sendProfileStats();
	void sendGoldDeagleClasses() noexcept;
	void writeMemory(DWORD dwAddress, void* bytes, DWORD dwSize);
	void unlockAll();
	void doLevel70();
	void doLevel1();
	void sendPrestige(int prestige);
	void sendRank();
	void doDLCMaps();
	void WriteBytes(LPVOID address, const char* bytes, int length);
	void doMaxPlayers(int amount);
	void doStartMatch();
	void doBalanceTeams();
	void FastRestart();
	void ChangeMap();
	void ChangeGamemode();
	void doForceHost();
	void doFFATeamFix();
	void handleHotkeys();
	void sendElevatorsToggle();
	void sendBouncesToggle();
	void sendBouncesToggleEasy();
	void giveAmmo();
	char* getPlayerName(int client);
	std::string readGameString(std::uintptr_t address, int maxLength = 256);
	int getLocalClientNum() noexcept;
	void sendCustomPort();
	void doIronSight();
	void writeSensitivity(float sens);
	float readSensitivity() noexcept;
	void menuUITweaks() noexcept;
	void sendSprintScale();
	void sendKnockbackScale();
	void sendBackSpeedScale();
	void sendPingText();
	void forceTeamChange();
	void sendUnlockAllClients();
	void loadPlayerNames();
	void DrawCrosshairOverlay() noexcept;
	std::vector<iw4::score_t> getScoreboardEntries();
	void NetworkFix() noexcept;
	void fuckTheCrosshairAway() noexcept;
	void sendViewModel() noexcept;
	bool isInGameNotSpectating();
}
