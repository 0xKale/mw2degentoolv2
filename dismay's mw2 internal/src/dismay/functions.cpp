#include "functions.hpp"


#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <atomic>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <thread>
#include <vector>
#include "../../ext/imgui/imgui.h"
#include "../menu/gui.hpp"
#include "../game/offsets.hpp"
#include "../game/iw4structs.hpp"
#include "../game/functions.hpp"
#include "../game/iw4hooks.h"
#include "../menu/pages/pages.hpp"
#include "../menu/pages/page_main.hpp"



DWORD dwordPlaceholder = static_cast<DWORD>(0xC);

static std::atomic<bool> g_featureWorkerStop{ true };
static std::thread g_featureWorker;

static void syncGameMouseCapture() noexcept
{
	if (gui::open)
		*reinterpret_cast<BYTE*>(iw4::offsets::mouse_enable) = 0;
	else
		*reinterpret_cast<BYTE*>(iw4::offsets::mouse_enable) = 1;
}

namespace functions {


	void syncImGuiMouseDrawCursor() noexcept
	{
		if (!ImGui::GetCurrentContext())
			return;
		ImGuiIO& io = ImGui::GetIO();
		io.WantCaptureKeyboard = gui::open;
		io.MouseDrawCursor = gui::open;
	}

	void handleMouseCursor()
	{
		syncGameMouseCapture();
		syncImGuiMouseDrawCursor();
	}

	void SetDvarInt(std::uintptr_t dvarAddress, int value)
	{
		DWORD dwPointer = *reinterpret_cast<DWORD*>(dvarAddress);
		*reinterpret_cast<int*>(dwPointer + iw4::pointers::Dvar) = value;
	}
	void SetDvarFloat(std::uintptr_t dvarAddress, float value)
	{
		DWORD dwPointer = *reinterpret_cast<DWORD*>(dvarAddress);
		*reinterpret_cast<float*>(dwPointer + iw4::pointers::Dvar) = value;
	}
	static int ReadDvarInt(std::uintptr_t dvarAddress)
	{
		std::uintptr_t dwPointer = *reinterpret_cast<std::uintptr_t*>(dvarAddress);
		if (!dwPointer)
		{
			return 0;
		}
		return *reinterpret_cast<int*>(dwPointer + iw4::pointers::Dvar);
	}
	static float ReadDvarFloat(std::uintptr_t dvarAddress)
	{
		std::uintptr_t dwPointer = *reinterpret_cast<std::uintptr_t*>(dvarAddress);
		if (!dwPointer)
		{
			return 0.0f;
		}
		float result = *reinterpret_cast<float*>(dwPointer + iw4::pointers::Dvar);
		return result;
	}
	int getHostId() noexcept
	{
		auto hostid = reinterpret_cast<int*>(iw4::offsets::hostId);
		return *hostid;
	}
	void fuckTheSunAway() noexcept
	{
		if (vars::noSun)
		{
			SetDvarInt(iw4::offsets::dvar::r_drawSun, 1);
		}
		else
		{
			SetDvarInt(iw4::offsets::dvar::r_drawSun, 0);
		}
	}
	void fuckTheCrosshairAway() noexcept
	{
		// Same toggle as DrawCrosshairOverlay: custom crosshair on => hide engine crosshair.
		const int want = vars::enableCrosshair ? 0 : 1;
		if (want == ReadDvarInt(iw4::offsets::dvar::cg_drawCrosshair))
			return;
		std::uintptr_t dwPointer = *reinterpret_cast<std::uintptr_t*>(iw4::offsets::dvar::cg_drawCrosshair);
		if (!dwPointer)
			return;
		SetDvarInt(iw4::offsets::dvar::cg_drawCrosshair, want);
	}
	void clearGlass() noexcept
	{
		if (!vars::clearGlass)
		{

			Cbuf_AddText(0, "glass_angular_vel 5 35;glass_edge_angle 5 10;glass_fall_delay 0.2 0.9;glass_fall_gravity 800;glass_fall_ratio 1.5 3;glass_fringe_maxcoverage 0.2;glass_fringe_maxsize 150;glass_fx_chance 0.25;glass_hinge_friction 50;glass_linear_vel 200 400;glass_max_pieces_per_frame 100;glass_max_shatter_fx_per_frame 6;glass_physics_chance 0.15;glass_physics_maxdist 512;glass_shard_maxsize 300;glass_shattered_scale 48;glass_trace_interval 100");
		}
		else
		{
			Cbuf_AddText(0, "glass_angular_vel 180 180;glass_edge_angle 5 10;glass_fall_delay 0 0;glass_fall_gravity 800;glass_fall_ratio 0 0;glass_fringe_maxcoverage 0;glass_fringe_maxsize 0;glass_fx_chance 0;glass_hinge_friction 0;glass_linear_vel 10000 10000;glass_max_pieces_per_frame 1;glass_max_shatter_fx_per_frame 1;glass_physics_chance 0;glass_physics_maxdist 0;glass_shard_maxsize 1;glass_shattered_scale 999999;glass_trace_interval 1");
		}
	}
	void sendNoCamo() noexcept
	{
		if (!vars::noCamo)
		{
			SetDvarInt(iw4::offsets::dvar::r_detail, 1);
			SetDvarInt(iw4::offsets::dvar::r_detailMap, 1);
		}
		else
		{
			SetDvarInt(iw4::offsets::dvar::r_detail, 0);
			SetDvarInt(iw4::offsets::dvar::r_detailMap, 0);
		}
	}
	void sendNoFog() noexcept
	{
		if (!vars::noFog)
		{
			SetDvarInt(iw4::offsets::dvar::r_fog, 0);
			SetDvarInt(iw4::offsets::dvar::fx_drawClouds, 0); // basically more fog.
			//SetDvarInt(iw4::offsets::dvar::r_detailMap, 0);
			//removed the other r_polygonOffsetScale replaced with r_detailMap
		}
		else
		{
			SetDvarInt(iw4::offsets::dvar::r_fog, 1);
			SetDvarInt(iw4::offsets::dvar::fx_drawClouds, 1);
			//SetDvarInt(iw4::offsets::dvar::r_detailMap, 1);
			//removed the other r_polygonOffsetScale replaced with r_detailMap
		}
	void sendNoBullets() noexcept
	{
		if (!vars::noBullets)
		{
			SetDvarInt(iw4::offsets::dvar::cg_brass, 1); // this is bullet casing coming out of the gun
			SetDvarInt(iw4::offsets::dvar::fx_marks, 1); // this is bullet casing coming out of the gun
		}
		else
		{
			SetDvarInt(iw4::offsets::dvar::cg_brass, 0);
			SetDvarInt(iw4::offsets::dvar::fx_marks, 0);
		}
	}
	void sendMovie() noexcept
	{
		if (!vars::movieMode)
		{
			SetDvarInt(iw4::offsets::dvar::r_filmUseTweaks, 1);
		}
		else
		{
			SetDvarInt(iw4::offsets::dvar::r_filmUseTweaks, 0);
		}
	}
	void sendFPSandFOV() noexcept
	{
		if (vars::framesPerSecond != ReadDvarInt(iw4::offsets::dvar::com_maxFPS))
		{
			SetDvarInt(iw4::offsets::dvar::com_maxFPS, vars::framesPerSecond);
		}
		if (vars::fieldOfView != ReadDvarFloat(iw4::offsets::dvar::cg_fov))
		{
			SetDvarFloat(iw4::offsets::dvar::cg_fov, vars::fieldOfView);
		}
	}
	void sendMapSize() noexcept
	{
		SetDvarFloat(iw4::offsets::dvar::compassSize, vars::mapSize);
	}

	namespace
	{
		bool isOneToOneExcludedWeapon() noexcept
		{
			const char* weapon = iw4::getCurrentWeaponName();
			if (weapon == nullptr)
			{
				return false;
			}
			return std::strstr(weapon, "cheytac") != nullptr
				|| std::strstr(weapon, "barrett") != nullptr
				|| std::strstr(weapon, "wa2000") != nullptr
				|| std::strstr(weapon, "m21") != nullptr;
		}
	}

	void sendFOVMin() noexcept
	{
		if (vars::enableMouseOneToOne && !isOneToOneExcludedWeapon())
		{
			if (vars::fieldOfView != ReadDvarFloat(iw4::offsets::dvar::cg_fovMin))
			{
				SetDvarFloat(iw4::offsets::dvar::cg_fovMin, vars::fieldOfView);
			}
		}
		else
		{
			SetDvarFloat(iw4::offsets::dvar::cg_fovMin, vars::defaultFovMin);
		}
	}
	void toggleChat() noexcept
	{
		if (vars::enableTextChat)
		{
			SetDvarInt(iw4::offsets::dvar::cg_chatTime, 12000);
		}
		else
		{
			SetDvarInt(iw4::offsets::dvar::cg_chatTime, 0);
		}
	}
	void mouseFix()
	{
		SetDvarInt(iw4::offsets::dvar::cl_bypassMouseInput, 0);
		SetDvarFloat(iw4::offsets::dvar::cl_mouseAccel, 0);
		//SetDvarFloat(iw4::offsets::dvar::cl_yawspeed, 0); // removed, this is not what you think it is. this is for key binded turning.
		//SetDvarFloat(iw4::offsets::dvar::cl_pitchspeed, 0); // I tink this is the same but for up and down. imma leave as it because I don't know for sure.
		SetDvarInt(iw4::offsets::dvar::m_filter, 0);
	}
 void NetworkFix() noexcept
    {
        int packets = 100;
        if (ReadDvarInt(iw4::offsets::dvar::cl_maxpackets) != packets)
        {
            SetDvarInt(iw4::offsets::dvar::cl_maxpackets, packets);
        }
        int packetdup = 5;
        if (ReadDvarInt(iw4::offsets::dvar::cl_packetdup) != packetdup)
        {
            SetDvarInt(iw4::offsets::dvar::cl_packetdup, packetdup);
        }
    }
	void doSaveBarracks()
	{
		*(int*)iw4::offsets::BarracksWins = vars::wins;
		*(int*)iw4::offsets::BarracksLosses = vars::losses;
		*(int*)iw4::offsets::BarracksTies = vars::ties;
		*(int*)iw4::offsets::BarracksWinStreak = vars::winStreak;
		*(int*)iw4::offsets::BarracksKills = vars::kills;
		*(int*)iw4::offsets::BarracksHeadshots = vars::headshots;
		*(int*)iw4::offsets::BarracksAssists = vars::assists;
		*(int*)iw4::offsets::BarracksKillStreak = vars::killStreak;
		*(int*)iw4::offsets::BarracksDeaths = vars::deaths;
		*(int*)iw4::offsets::BarracksTimePlayed = vars::timePlayed;
	}
	void loadProfileStats() noexcept
	{
		vars::wins = *(int*)iw4::offsets::BarracksWins;
		vars::losses = *(int*)iw4::offsets::BarracksLosses;
		vars::ties = *(int*)iw4::offsets::BarracksTies;
		vars::winStreak = *(int*)iw4::offsets::BarracksWinStreak;
		vars::kills = *(int*)iw4::offsets::BarracksKills;
		vars::headshots = *(int*)iw4::offsets::BarracksHeadshots;
		vars::assists = *(int*)iw4::offsets::BarracksAssists;
		vars::killStreak = *(int*)iw4::offsets::BarracksKillStreak;
		vars::deaths = *(int*)iw4::offsets::BarracksDeaths;
		vars::timePlayed = *(int*)iw4::offsets::BarracksTimePlayed;
	}
	void sendGoldDeagleClasses() noexcept
	{
		*(int*)0x1B8BB7C = 327776; // class 2
		*(int*)0x1B8BBBC = 327776; // class 3
		*(int*)0x1B8BBFC = 327776; // class 4
		*(int*)0x1B8BC7C = 327776; // class 6
		*(int*)0x1B8BCFC = 327776; // class 8
		*(int*)0x1B8BD3C = 327776; // class 9
	}
	void writeMemory(DWORD dwAddress, void* bytes, DWORD dwSize)
	{
		DWORD flOldProtect = 0;

		// Change memory protection to allow writing
		VirtualProtect(reinterpret_cast<void*>(dwAddress), dwSize, PAGE_EXECUTE_READWRITE, &flOldProtect);

		// Write the bytes
		memcpy(reinterpret_cast<void*>(dwAddress), bytes, dwSize);

		// Restore the original memory protection
		VirtualProtect(reinterpret_cast<void*>(dwAddress), dwSize, flOldProtect, &flOldProtect);
	}
	void unlockAll()
	{
		const uint8_t NOP = 0x90;
		uint8_t* unlockAll = (uint8_t*)malloc(2572);
		if (unlockAll != NULL)
		{
			memset(unlockAll, NOP, 2572);
			writeMemory(0x01B8BD8F, unlockAll, 2572);
			free(unlockAll);
		}
	}
	static int xpToAdvanceFromRank(int n) noexcept
	{
		// XP to go from rank n -> n+1 (MW2 table / piecewise segments).
		if (n < 1 || n > 70)
			return 0;
		if (n <= 10)
			return 500 + 700 * (n - 1);
		if (n <= 29)
			return 7800 + 1000 * (n - 11);
		if (n <= 49)
			return 27000 + 1200 * (n - 30);
		return 51300 + 1500 * (n - 50);
	}

	static DWORD totalXpAtRank(int rank) noexcept
	{
		// Cumulative XP when you have just reached `rank` (same as table "Total XP Required").
		if (rank <= 1)
			return 0;
		if (rank > 70)
			rank = 70;
		DWORD sum = 0;
		for (int i = 1; i < rank; ++i)
			sum += static_cast<DWORD>(xpToAdvanceFromRank(i));
		return sum;
	}

	void doLevel70()
	{
		*reinterpret_cast<DWORD*>(iw4::offsets::LocalClientLevel) = totalXpAtRank(70);
	}
	void doLevel1()
	{
		*(DWORD*)iw4::offsets::LocalClientLevel = 0;
	}
	void sendPrestige(int prestige)
	{
		*(DWORD*)iw4::offsets::LocalClientPrestige = prestige;
	}
	void doDLCMaps()
	{
		static constexpr iw4::DLCDef originalMaps[] = {
			{ 2, "MP_ORIGINAL_MAPS" },
		};
		static constexpr iw4::DLCDef allMaps[] = {
			{ 2, "MP_ORIGINAL_MAPS" },
			{ 4, "DLC_1" },
			{ 8, "DLC_2" },
		};

		const iw4::DLCDef* items = originalMaps;
		int itemCount = static_cast<int>(sizeof(originalMaps) / sizeof(originalMaps[0]));
		const int maxItems = static_cast<int>(sizeof(allMaps) / sizeof(allMaps[0]));

		if (vars::enableDLC)
		{
			items = allMaps;
			itemCount = maxItems;
		}

		for (int i = 0; i < maxItems; ++i)
		{
			iw4::DLCList item{};
			if (i < itemCount)
			{
				item.a2 = items[i].a2;
				item.flag1 = 1;
				item.flag2 = 1;
				strncpy_s(item.name, sizeof(item.name), items[i].name, _TRUNCATE);
			}

			const auto address = iw4::offsets::dlc_location + static_cast<std::uintptr_t>(i) * sizeof(iw4::DLCList);
			WriteBytes((LPVOID)address, reinterpret_cast<const char*>(&item), sizeof(item));
		}

		WriteBytes((LPVOID)iw4::offsets::dlc_count, reinterpret_cast<const char*>(&itemCount), sizeof(itemCount));
	}
	void WriteBytes(LPVOID address, const char* bytes, int length)
	{
		DWORD origProtect;
		VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &origProtect);
		memcpy(address, bytes, length);
	}

	void doMaxPlayers(int amount)
	{
		if (amount < 2 || amount > 18) return;

		std::string player = std::to_string(amount);
		std::string command = "sv_maxclients " + player + ";party_maxplayers " + player + ";ui_maxclients " + player;
		Cbuf_AddText(0, command.c_str());

	}
	void doStartMatch()
	{
		Cbuf_AddText(0, ";xblive_privatematch 1;wait 2;xpartygo;wait 2;xblive_privatematch 0;");
	}

	void doBalanceTeams()
	{
		BalanceTeams(reinterpret_cast<void*>(iw4::offsets::G_LOBBYDATA));
		BalanceTeams(reinterpret_cast<void*>(iw4::offsets::PARTYSESSION_P));
	}
	void FastRestart()
	{
		MapRestart(0, 0);
		return;
	}
	void ChangeMap()
	{
		SV_SpawnServer((char*)vars::mapList[vars::selectedMap], 0, 0);
		return;
	}

	void ChangeGamemode()
	{
		switch (vars::selectedGamemode)
		{
		case 0:
			Cbuf_AddText(0, "g_gametype dom; ui_gametype dom; party_gametype dom");
			break;
		case 1:
			Cbuf_AddText(0, "g_gametype war; ui_gametype war; party_gametype war");
			break;
		case 2:
			Cbuf_AddText(0, "g_gametype sd; ui_gametype sd; party_gametype sd");
			break;
		case 3:
			Cbuf_AddText(0, "g_gametype ffa; ui_gametype ffa; party_gametype ffa");
			break;
		case 4:
			Cbuf_AddText(0, "g_gametype koth; ui_gametype koth; party_gametype koth");
			break;
		case 5:
			Cbuf_AddText(0, "g_gametype dem; ui_gametype dem; party_gametype dem");
			break;
		case 6:
			Cbuf_AddText(0, "g_gametype sab; ui_gametype sab; party_gametype sab");
			break;
		case 7:
			Cbuf_AddText(0, "g_gametype ctf; ui_gametype ctf; party_gametype ctf");
			break;
		case 8:
			Cbuf_AddText(0, "g_gametype gtnw; ui_gametype gtnw; party_gametype gtnw");
			break;
		case 9:
			Cbuf_AddText(0, "g_gametype oneflag; ui_gametype oneflag; party_gametype oneflag");
			break;
		case 10:
			Cbuf_AddText(0, "g_gametype vip; ui_gametype vip; party_gametype vip");
			break;
		case 11:
			Cbuf_AddText(0, "g_gametype arena; ui_gametype arena; party_gametype arena");
			break;
		default:
			Cbuf_AddText(0, "g_gametype dom; ui_gametype dom; party_gametype dom"); // default to domination if failed
			break;
		}
	}
	void doForceHost()
	{
		Cbuf_AddText(0, "party_connectTimeout 1000");
		Cbuf_AddText(0, "party_connectTimeout 1");
		Cbuf_AddText(0, "party_host 1");
		Cbuf_AddText(0, "party_hostmigration 0");
		Cbuf_AddText(0, "onlinegame 1");
		Cbuf_AddText(0, "onlinegameandhost 1");
		Cbuf_AddText(0, "onlineunrankedgameandhost 0");
		Cbuf_AddText(0, "migration_msgtimeout 0");
		Cbuf_AddText(0, "migration_timeBetween 999999");
		Cbuf_AddText(0, "migration_verboseBroadcastTime 0");
		Cbuf_AddText(0, "migrationPingTime 0");
		Cbuf_AddText(0, "bandwidthtest_duration 0");
		Cbuf_AddText(0, "bandwidthtest_enable 0");
		Cbuf_AddText(0, "bandwidthtest_ingame_enable 0");
		Cbuf_AddText(0, "bandwidthtest_timeout 0");
		Cbuf_AddText(0, "cl_migrationTimeout 0");
		Cbuf_AddText(0, "lobby_partySearchWaitTime 0");
		Cbuf_AddText(0, "bandwidthtest_announceinterval 0");
		Cbuf_AddText(0, "partymigrate_broadcast_interval 99999");
		Cbuf_AddText(0, "partymigrate_pingtest_timeout 0");
		Cbuf_AddText(0, "partymigrate_timeout 0");
		Cbuf_AddText(0, "partymigrate_timeoutmax 0");
		Cbuf_AddText(0, "partymigrate_pingtest_retry 0");
		Cbuf_AddText(0, "partymigrate_pingtest_timeout 0");
		Cbuf_AddText(0, "g_kickHostIfIdle 0");
		Cbuf_AddText(0, "sv_cheats 1");
		Cbuf_AddText(0, "xblive_playEvenIfDown 1");
		Cbuf_AddText(0, "party_hostmigration 0");
		Cbuf_AddText(0, "badhost_endGameIfISuck 0");
		Cbuf_AddText(0, "badhost_maxDoISuckFrames 0");
		Cbuf_AddText(0, "badhost_maxHappyPingTime 99999");
		Cbuf_AddText(0, "badhost_minTotalClientsForHappyTest 99999");
		Cbuf_AddText(0, "bandwidthtest_enable 0");
	}

	void doFFATeamFix()
	{
		if (vars::FFATeamFix)
		{
			for (int i = 0; i < 18; i++)
			{
				*reinterpret_cast<int*>(0x1B1139C - 0x80 + (0x366C * i)) = 0x00;
			}
		}
	}

	bool noclip = false;

	void handleHotkeys()
	{
		if (!vars::enableHostHotkeys)
			return;

		if (GetAsyncKeyState(VK_F2) & 1) // F2
		{
			Cbuf_AddText(0, reinterpret_cast<const char*>(0x00AB2D88));
		}
		if (GetAsyncKeyState(VK_F3) & 1) // F3
		{
			functions::doForceHost();
		}
		if (GetAsyncKeyState(VK_F4) & 1) // F4
		{
			functions::ChangeGamemode();
			OpenMenu(0, "popup_gamesetup");
			Cbuf_AddText(0, "xblive_privatematch 1");
		}
		if (GetAsyncKeyState(VK_F5) & 1) // F5
		{
			functions::ChangeGamemode();
			functions::doMaxPlayers(vars::maxPlayers);
			functions::doStartMatch();
			functions::doBalanceTeams();
		}
		if (GetAsyncKeyState(88) & 1) // Key: X
		{
			noclip = !noclip;
			if (noclip)
			{
				*reinterpret_cast<int*>(0x1B114D4 + (getHostId() * 0x366C)) = 0x01;
			}
			if (!noclip)
			{
				*reinterpret_cast<int*>(0x1B114D4 + (getHostId() * 0x366C)) = 0x00;
			}
		}
	}
	void sendElevatorsToggle()
	{
		const char elevatorsDepatch[] = { (char)0xEB, (char)0x42 };
		const char elevatorsOriginal[] = { (char)0x4A, (char)0x2A };
		if (vars::enableDepatchElevators)
		{
			WriteBytes((LPVOID)0x00471329, elevatorsDepatch, sizeof(elevatorsDepatch));
		}
		else
		{
			WriteBytes((LPVOID)0x00471329, elevatorsOriginal, sizeof(elevatorsOriginal));
		}
	}

	void sendBouncesToggle()
	{
		const char bounceDepatch[] = { (char)0x90, (char)0x90 };
		const char bounceOriginal[] = { (char)0x75, (char)0x14 };
		if (vars::enableDepatchBounces)
		{
			WriteBytes((LPVOID)0x004736E2, bounceDepatch, sizeof(bounceDepatch));
		}
		else
		{
			WriteBytes((LPVOID)0x004736E2, bounceOriginal, sizeof(bounceOriginal));
		}
	}
	void sendBouncesToggleEasy()
	{
		// these do not count as clips lmao i swear to god
		// Thanks https://github.com/V3nilla
		const char bounceDepatch[] = {(char)0x74, (char)0x14};
        const char bounceDepatch2[] = {(char)0xEB, (char)0x35};
        const char bounceOriginal[] = {(char)0x75, (char)0x14};
        const char bounceOriginal2[] = {(char)0x75, (char)0x35};
        if (vars::enableDepatchBouncesEasy)
        {
            WriteBytes((LPVOID)0x004736E2, bounceDepatch, sizeof(bounceDepatch));
            WriteBytes((LPVOID)0x004736F6, bounceDepatch2, sizeof(bounceDepatch2));
        }
        else
        {
            WriteBytes((LPVOID)0x004736E2, bounceOriginal, sizeof(bounceOriginal));
            WriteBytes((LPVOID)0x004736F6, bounceOriginal2, sizeof(bounceOriginal2));
        }
	}

	char* getPlayerName(int client)
	{
		static char kEmpty[1] = { '\0' };
		if (client < 0 || client > 17)
			return kEmpty;
		return reinterpret_cast<char*>(0x99786C + (client * 0x52C));
	}

	void loadPlayerNames()
	{
		char gameName[40];
		for (int i = 0; i <= 17; ++i) {
			const char* src = getPlayerName(i);
			gameName[0] = '\0';
			__try {
				unsigned k = 0;
				while (k < sizeof(gameName) - 1u && src[k] != '\0')
					++k;
				for (unsigned j = 0; j < k; ++j)
					gameName[j] = src[j];
				gameName[k] = '\0';
			}
			__except (EXCEPTION_EXECUTE_HANDLER) {
				gameName[0] = '\0';
			}
			sprintf_s(vars::playerName[i], sizeof(vars::playerName[i]), "Client(%d)%s", i, gameName);
		}
	}

	void sendCustomPort()
	{
		SetDvarInt(iw4::offsets::dvar::net_port, vars::customPort);
	}

	void doIronSight()
	{
		BYTE bytes1[2] = { 0x00, 0x00 };
		BYTE bytes2[1] = { 0x00 };
		BYTE bytes3[2] = { 0x3E, 0x16 };
		BYTE bytes4[1] = { 0x40 };

		if (vars::ironSightIntervention)
		{
			SetDvarFloat(iw4::offsets::dvar::cg_gun_z, 1.0f);
			SetDvarFloat(iw4::offsets::dvar::cg_gun_y, 0.0f);
			SetDvarFloat(iw4::offsets::dvar::cg_gun_x, -1.0f);
			memcpy((void*)0x2516392D, bytes1, 2);
			memcpy((void*)0x251639AD, bytes1, 2);
			memcpy((void*)0x2516396D, bytes1, 2);
			memcpy((void*)0x251639ED, bytes1, 2);
			memcpy((void*)0x25162F1F, bytes2, 1);
			strcpy_s(reinterpret_cast<char*>(0x33CB8FEC), 28, "Intervention IRON Sight");
		}
		else
		{
			SetDvarFloat(iw4::offsets::dvar::cg_gun_z, 0.0f);
			SetDvarFloat(iw4::offsets::dvar::cg_gun_y, 0.0f);
			SetDvarFloat(iw4::offsets::dvar::cg_gun_x, 0.0f);
			memcpy((void*)0x2516392D, bytes3, 2);
			memcpy((void*)0x251639AD, bytes3, 2);
			memcpy((void*)0x2516396D, bytes3, 2);
			memcpy((void*)0x251639ED, bytes3, 2);
			memcpy((void*)0x25162F1F, bytes4, 1);
			strcpy_s(reinterpret_cast<char*>(0x33CB8FEC), 28, "Intervention ACOG Sight");
		}
	}

	void writeSensitivity(float sens)
	{
		// writeMemory(0x063832DC, &sens, sizeof(float)); // I am not sure why this doesn't work
		std::string cmd = "sensitivity " + std::to_string(sens) + ";";
		Cbuf_AddText(0, cmd.c_str());
	}

	float readSensitivity() noexcept
	{
		float val = ReadDvarFloat(iw4::offsets::dvar::sensitivity);
		// std::cout << "[Sensitivity] Read: " << val << std::endl;
		return val;
	}

	void menuUITweaks() noexcept
	{
		// main menu tweaks
		strcpy_s(reinterpret_cast<char*>(0x33BD7AF4), 28, "^1d^7ismay's ^1degen^7 tool"); // ITnet
		strcpy_s(reinterpret_cast<char*>(0x33BD80C0), 28, "^2FIND GAME");                 // Find game
		strcpy_s(reinterpret_cast<char*>(0x33BD8519), 28, "^1PRIVATE SESH");              // private match
		strcpy_s(reinterpret_cast<char*>(0x33BD8A99), 28, "^3CREATE A CLASS");            // create a class
		strcpy_s(reinterpret_cast<char*>(0x33BD9BE0), 28, "^4STREAKS/CALLSIGNS");         // killstreaks
		strcpy_s(reinterpret_cast<char*>(0x33BDA69E), 28, "^5BARRACKS OBAMA");            // barracks
		strcpy_s(reinterpret_cast<char*>(0x33BDAFE1), 28, "^6INVITE PEEPS");              // invite
		strcpy_s(reinterpret_cast<char*>(0x33BE3F80), 28, "^0PLAYLISTS"); // playlists

		strcpy_s(reinterpret_cast<char*>(0x33C0759C), 28, "^2START");    // start game
		strcpy_s(reinterpret_cast<char*>(0x33C079A5), 28, "^1SETTINGS"); // settings
		// title iron lungs
		strcpy_s(reinterpret_cast<char*>(0x33CBD9E6), 28, "dismay >");
	}

	static void FunctionWorkerLoop() noexcept
	{
		ULONGLONG lastMenuTweakMs = 0;
		while (!g_featureWorkerStop.load(std::memory_order_acquire))
		{
			if (!gui::setup)
			{
				::Sleep(10);
				continue;
			}

			syncGameMouseCapture();
			sendFPSandFOV();
			fuckTheCrosshairAway();
			doDLCMaps();
			doFFATeamFix();
			handleHotkeys();
			sendMapSize();
			mouseFix();
			NetworkFix();
			if (vars::enableMouseOneToOne)
			{
				sendFOVMin();
			}

			const ULONGLONG now = GetTickCount64();
			if (now - lastMenuTweakMs >= 250ULL)
			{
				menuUITweaks();
				lastMenuTweakMs = now;
			}

			// ~1000 Hz max; avoids a tight spin that steals a full core from the game.
			::Sleep(1);
		}
	}

	void startFeatureWorker() noexcept
	{
		stopFeatureWorker();
		g_featureWorkerStop.store(false, std::memory_order_release);
		g_featureWorker = std::thread(FunctionWorkerLoop);
	}

	void stopFeatureWorker() noexcept
	{
		g_featureWorkerStop.store(true, std::memory_order_release);
		if (g_featureWorker.joinable())
			g_featureWorker.join();
	}

	void sendPingText()
	{
		if (vars::pingText)
		{
			SetDvarInt(iw4::offsets::dvar::cg_scoreboardPingText, 1);
		}
		else
		{
			SetDvarInt(iw4::offsets::dvar::cg_scoreboardPingText, 0);
		}
	}
	void sendProfileStats()
	{
		doSaveBarracks();
	}
	void giveAmmo()
	{
		for (int i = 0; i < 18; i++)
		{
			*reinterpret_cast<int*>(iw4::offsets::PrimaryMagAmmo + (i * 0x366C)) = 999;
			*reinterpret_cast<int*>(iw4::offsets::PrimaryReserveAmmo + (i * 0x366C)) = 999;
			*reinterpret_cast<int*>(iw4::offsets::SecondaryAmmoReserve + (i * 0x366C)) = 999;
			*reinterpret_cast<int*>(iw4::offsets::SecondaryLeftGunMagAmmo + (i * 0x366C)) = 999;
			*reinterpret_cast<int*>(iw4::offsets::SecondaryRightGunMagAmmo + (i * 0x366C)) = 999;
			*reinterpret_cast<int*>(iw4::offsets::PrimNade + (i * 0x366C)) = 999;
			*reinterpret_cast<int*>(iw4::offsets::StunNade + (i * 0x366C)) = 999;
		}
	}
	void sendRank()
	{
		*reinterpret_cast<DWORD*>(iw4::offsets::LocalClientLevel) = totalXpAtRank(vars::rank);
	}
	void sendSprintScale()
	{
		SetDvarFloat(iw4::offsets::dvar::player_sprintSpeedScale, vars::sprintScale);
	}
	void sendKnockbackScale()
	{
		SetDvarFloat(iw4::offsets::dvar::g_knockback, vars::knockbackScale);
	}
	void sendBackSpeedScale()
	{
		SetDvarFloat(iw4::offsets::dvar::player_backSpeedScale, vars::backSpeedScale);
	}

	void sendViewModel() noexcept
	{
		SetDvarFloat(iw4::offsets::dvar::cg_gun_x, vars::fcg_gun_x);
		SetDvarFloat(iw4::offsets::dvar::cg_gun_y, vars::fcg_gun_y);
		SetDvarFloat(iw4::offsets::dvar::cg_gun_z, vars::fcg_gun_z);
	}
	char bullshit[30];
	void forceTeamChange()
	{
		sprintf_s(bullshit, "mr %i 2 spectator\n", reinterpret_cast<int*>(iw4::offsets::bullshit2)[0]);
		Cbuf_AddText(0, bullshit);
	}

	void sendUnlockAllClients()
	{
		for (int i = 0; i <= 17; ++i) {

             SV_GameSendServerCommand(i, 0, (char*)"s loc_warningsUI \"0\"");
             SV_GameSendServerCommand(i, 0, (char*)"c \"^2Unlocking All Challenges Now!\"");
             SV_GameSendServerCommand(i, 0, (char*)"J 3760 09 4623 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3761 09 4627 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3762 02 4631 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3763 02 4635 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3764 02 4639 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3765 02 4643 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3766 02 4647 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3767 02 4651 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3752 09 4591 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3753 09 4595 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3754 02 4599 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3755 02 4603 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3756 02 4607 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3757 02 4611 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3758 02 4615 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3759 02 4619 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3736 09 4527 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3737 09 4531 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3738 02 4535 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3739 02 4539 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3740 02 4543 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3741 02 4547 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3742 02 4551 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3743 02 4555 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3799 09 4779 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3800 09 4783 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3801 02 4787 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3802 02 4791 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3803 02 4795 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3804 02 4799 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3805 02 4803 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3806 02 4807 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3775 09 4683 E803");

             SV_GameSendServerCommand(i, 0, (char*)"c \"^2Unlock All ^425 Percent Done!\"");
             SV_GameSendServerCommand(i, 0, (char*)"J 3776 09 4687 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3777 02 4691 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3778 02 4695 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3779 02 4699 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3780 02 4703 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3781 02 4707 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3782 02 4711 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3728 09 4495 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3729 09 4499 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3730 02 4503 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3731 02 4507 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3732 02 4511 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3733 02 4515 14");

             SV_GameSendServerCommand(i, 0, (char*)"J 3734 02 4519 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3735 02 4523 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3783 09 4715 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3784 09 4719 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3785 02 4723 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3786 02 4727 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3787 02 4731 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3788 02 4735 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3789 02 4739 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3790 02 4743 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3791 09 4747 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3792 09 4751 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3793 02 4755 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3794 02 4759 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3795 02 4763 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3796 02 4767 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3797 02 4771 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3798 02 4775 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3744 09 4559 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3745 09 4563 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3746 02 4567 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3747 02 4571 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3748 02 4575 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3749 02 4579 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3750 02 4583 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3751 02 4587 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3853 09 4995 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3854 09 4999 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3855 02 5003 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 3856 02 5007 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3857 02 5011 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3858 02 5015 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3859 02 5019 09");
             SV_GameSendServerCommand(i, 0, (char*)"J 3839 09 4939 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3840 09 4943 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3841 02 4947 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 3842 02 4951 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3843 02 4955 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3844 02 4959 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3845 02 4963 09");
             SV_GameSendServerCommand(i, 0, (char*)"J 3825 09 4883 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3826 09 4887 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3827 02 4891 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 3828 02 4895 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3829 02 4899 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3830 02 4903 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3831 02 4907 09");
             SV_GameSendServerCommand(i, 0, (char*)"J 3832 09 4911 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3833 09 4915 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3834 02 4919 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 3835 02 4923 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3836 02 4927 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3837 02 4931 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3838 02 4935 09");

             SV_GameSendServerCommand(i, 0, (char*)"J 3846 09 4967 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3847 09 4971 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3848 02 4975 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 3849 02 4979 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3850 02 4983 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3851 02 4987 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3852 02 4991 09");
             SV_GameSendServerCommand(i, 0, (char*)"J 3768 09 4655 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3769 09 4659 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3771 02 4667 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3770 02 4663 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3772 02 4671 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3773 02 4675 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3774 02 4679 09");
             SV_GameSendServerCommand(i, 0, (char*)"J 3874 09 5079 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3875 09 5083 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3876 02 5087 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3877 02 5091 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3878 02 5095 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3879 02 5099 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3880 02 5103 09");
             SV_GameSendServerCommand(i, 0, (char*)"J 3867 09 5051 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3868 09 5055 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3869 02 5059 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3870 02 5063 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3871 02 5067 14");

             SV_GameSendServerCommand(i, 0, (char*)"c \"^2Unlock All ^450 Percent Done!\"");
             SV_GameSendServerCommand(i, 0, (char*)"J 3872 02 5071 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3873 02 5075 09");
             SV_GameSendServerCommand(i, 0, (char*)"J 3860 09 5023 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3861 09 5027 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3862 02 5031 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3863 02 5035 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3864 02 5039 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3865 02 5043 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3866 02 5047 09");
             SV_GameSendServerCommand(i, 0, (char*)"J 3888 09 5135 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3887 09 5131 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3889 02 5139 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3890 02 5143 3C");
             SV_GameSendServerCommand(i, 0, (char*)"J 3891 02 5147 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3892 02 5151 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3893 02 5155 09");
             SV_GameSendServerCommand(i, 0, (char*)"J 3807 09 4811 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3808 09 4815 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3809 02 4819 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3810 02 4823 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3811 02 4827 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3812 02 4831 06");
             SV_GameSendServerCommand(i, 0, (char*)"J 3813 09 4835 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3814 09 4839 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3815 02 4843 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3816 02 4847 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3817 02 4851 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3818 02 4855 06");
             SV_GameSendServerCommand(i, 0, (char*)"J 3819 09 4859 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3820 09 4863 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3821 02 4867 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3822 02 4871 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3823 02 4875 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3824 02 4879 06");
             SV_GameSendServerCommand(i, 0, (char*)"J 3881 09 5107 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3882 09 5111 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3883 02 5115 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 3884 02 5119 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3885 02 5123 28");
             SV_GameSendServerCommand(i, 0, (char*)"J 3886 02 5127 06");
             SV_GameSendServerCommand(i, 0, (char*)"J 3898 09 5175 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3899 09 5179 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3894 09 5159 E803");

             SV_GameSendServerCommand(i, 0, (char*)"J 3895 09 5163 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3900 09 5183 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3901 09 5187 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3896 09 5167 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3897 09 5171 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3902 09 5191 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3903 09 5195 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3908 09 5215 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3909 09 5219 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3904 09 5199 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3905 09 5203 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3906 09 5207 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3907 09 5211 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3912 06 5231 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3913 09 5235 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3910 06 5223 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3911 09 5227 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3916 09 5247 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3917 09 5251 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3914 09 5239 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3915 09 5243 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3920 07 5263 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3921 09 5267 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3918 07 5255 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3919 09 5259 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3922 09 5271 B004");
             SV_GameSendServerCommand(i, 0, (char*)"J 3923 09 5275 B004");
             SV_GameSendServerCommand(i, 0, (char*)"J 3924 09 5279 B004");
             SV_GameSendServerCommand(i, 0, (char*)"J 3925 09 5283 B004");
             SV_GameSendServerCommand(i, 0, (char*)"J 3926 09 5287 FA");
             SV_GameSendServerCommand(i, 0, (char*)"J 3643 0A 4155 09");
             SV_GameSendServerCommand(i, 0, (char*)"J 4122 02 6071 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3652 04 4191 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3653 04 4195 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3650 04 4183 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3651 04 4187 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3646 04 4167 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3647 04 4171 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3662 04 4231 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3663 04 4235 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3656 04 4207 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3657 04 4211 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3644 04 4159 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3645 04 4163 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3658 04 4215 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3659 04 4219 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3660 04 4223 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3661 04 4227 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3648 04 4175 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3649 04 4179 E803");

             SV_GameSendServerCommand(i, 0, (char*)"J 3678 04 4295 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3679 04 4299 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3674 04 4279 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3675 04 4283 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3670 04 4263 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3671 04 4267 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3672 04 4271 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3673 04 4275 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3676 04 4287 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3677 04 4291 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3654 04 4199 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3655 04 4203 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3684 04 4319 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3685 04 4323 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3682 04 4311 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3683 04 4315 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3680 04 4303 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3681 04 4307 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3716 04 4447 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3717 04 4451 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3664 04 4239 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3665 04 4243 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3666 04 4247 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3667 04 4251 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3668 04 4255 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3669 04 4259 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3714 04 4439 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3715 04 4443 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3690 04 4343 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3691 04 4347 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3686 04 4327 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3687 04 4331 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3692 04 4351 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3693 04 4355 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3688 04 4335 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3689 04 4339 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3694 04 4359 C409");

             SV_GameSendServerCommand(i, 0, (char*)"c \"^2Unlock All ^475 Percent Done!\"");
             SV_GameSendServerCommand(i, 0, (char*)"J 3695 04 4363 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3700 04 4383 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3701 04 4387 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3696 04 4367 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3697 04 4371 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3698 04 4375 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3699 04 4379 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3704 04 4399 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3705 04 4403 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3702 04 4391 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3703 04 4395 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3708 04 4415 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3709 04 4419 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3706 04 4407 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3707 04 4411 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3712 04 4431 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3713 04 4435 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3710 04 4423 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3711 04 4427 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3718 04 4455 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3719 04 4459 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3720 04 4463 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3721 04 4467 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3722 04 4471 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3723 04 4475 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3724 04 4479 C409");
             SV_GameSendServerCommand(i, 0, (char*)"J 3725 04 4483 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3726 05 4487 8813");
             SV_GameSendServerCommand(i, 0, (char*)"J 3727 04 4491 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3927 07 5292 6108");
             SV_GameSendServerCommand(i, 0, (char*)"J 3931 07 5307 EE02");

             SV_GameSendServerCommand(i, 0, (char*)"J 3938 07 5335 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3932 07 5311 8403");
             SV_GameSendServerCommand(i, 0, (char*)"J 3935 07 5323 EE02");
             SV_GameSendServerCommand(i, 0, (char*)"J 3933 07 5315 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3941 07 5347 402414");
             SV_GameSendServerCommand(i, 0, (char*)"J 3934 07 5319 FA");
             SV_GameSendServerCommand(i, 0, (char*)"J 3936 07 5327 FA");
             SV_GameSendServerCommand(i, 0, (char*)"J 3942 07 5351 F4O");

             SV_GameSendServerCommand(i, 0, (char*)"J 3939 07 5339 64");
             SV_GameSendServerCommand(i, 0, (char*)"J 3928 07 5295 F4O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3930 07 5303 FA");
             SV_GameSendServerCommand(i, 0, (char*)"J 3929 07 5299 FA");
             SV_GameSendServerCommand(i, 0, (char*)"J 3940 07 5343 EE02");
             SV_GameSendServerCommand(i, 0, (char*)"J 3937 07 5331 64");
             SV_GameSendServerCommand(i, 0, (char*)"J 3943 04 5355 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 3944 04 5359 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 3945 04 5363 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 3946 04 5367 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 3947 04 5371 32");

             SV_GameSendServerCommand(i, 0, (char*)"J 3948 04 5375 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 3949 04 5379 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 3950 04 5383 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 3951 04 5387 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 3952 04 5391 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 3953 04 5395 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 3954 04 5399 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 3955 04 5403 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 3956 04 5407 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3957 04 5411 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 3958 04 5415 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3959 04 5419 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3960 04 5423 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3961 04 5427 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3962 04 5431 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 3963 04 5435 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 3964 04 5439 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 3965 04 5443 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 3966 04 5447 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 3967 04 5451 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 3968 04 5455 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 3969 02 5459 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3970 02 5463 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 3971 02 5467 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 3972 02 5471 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3973 02 5475 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3974 05 5479 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3975 05 5483 E803");

             SV_GameSendServerCommand(i, 0, (char*)"J 3976 05 5487 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3977 05 5491 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3978 05 5495 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3979 05 5499 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3980 05 5503 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3981 05 5507 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3982 05 5511 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 3983 02 5515 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3984 02 5519 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3985 02 5523 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3986 02 5527 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3987 02 5531 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3988 02 5535 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3989 02 5539 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3990 02 5543 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3991 02 5547 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3992 02 5551 O");

             SV_GameSendServerCommand(i, 0, (char*)"J 3993 04 5555 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 3994 02 5559 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3995 02 5563 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3996 02 5567 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3997 02 5571 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 3998 04 5575 C8");
             SV_GameSendServerCommand(i, 0, (char*)"J 3999 03 5579 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 4000 03 5583 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 4001 02 5587 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4002 02 5591 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4003 02 5595 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4004 02 5599 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4005 02 5603 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4006 02 5607 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4007 02 5611 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4008 02 5615 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4009 02 5619 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4010 02 5623 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4011 02 5627 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4012 02 5631 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4013 02 5635 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4014 02 5639 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4015 02 5643 O");

             SV_GameSendServerCommand(i, 0, (char*)"J 4016 02 5647 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4017 02 5651 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4018 02 5655 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4019 02 5659 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4020 04 5663 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 4021 04 5667 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 4022 04 5671 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 4023 04 5675 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 4024 04 5679 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 4025 04 5683 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 4026 02 5687 O");

             SV_GameSendServerCommand(i, 0, (char*)"J 4027 02 5691 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4028 04 5695 50C3");
             SV_GameSendServerCommand(i, 0, (char*)"J 4029 04 5699 50C3");
             SV_GameSendServerCommand(i, 0, (char*)"J 4030 04 5703 64");
             SV_GameSendServerCommand(i, 0, (char*)"J 4031 02 5707 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4032 02 5711 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4033 02 5715 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4034 02 5719 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4035 04 5723 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 4036 04 5727 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 4037 04 5731 32");

             SV_GameSendServerCommand(i, 0, (char*)"J 4038 04 5735 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 4039 04 5739 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 4040 04 5743 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 4041 04 5747 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 4042 02 5751 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4043 02 5755 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4044 02 5759 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4045 02 5763 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4046 02 5767 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4047 02 5771 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4048 02 5775 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4049 02 5779 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4050 03 5783 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 4051 03 5787 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 4052 02 5791 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4053 02 5795 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4054 02 5799 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4055 03 5803 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 4056 03 5807 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 4057 02 5811 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4058 03 5815 19");
             SV_GameSendServerCommand(i, 0, (char*)"J 4059 02 5819 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4060 02 5823 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4061 02 5827 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4062 02 5831 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4063 02 5835 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4064 02 5839 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4065 04 5843 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 4066 02 5847 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4067 02 5851 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4068 04 5855 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 4069 04 5859 14");
             SV_GameSendServerCommand(i, 0, (char*)"J 4070 02 5863 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4071 02 5867 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4072 02 5871 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4073 02 5875 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4074 02 5879 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4075 02 5883 O");

             SV_GameSendServerCommand(i, 0, (char*)"J 4076 02 5887 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4077 02 5891 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4078 02 5895 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4079 02 5899 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4080 02 5903 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4081 02 5907 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4082 02 5911 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4083 02 5915 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4084 02 5919 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4085 02 5923 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4086 02 5927 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4087 02 5931 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4088 02 5935 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4089 02 5939 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4090 02 5943 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4091 02 5947 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4092 02 5951 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4093 02 5955 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4094 02 5959 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4095 02 5963 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4096 02 5967 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4097 02 5971 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4098 02 5975 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4099 02 5979 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4100 02 5983 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4101 04 5987 C8");
             SV_GameSendServerCommand(i, 0, (char*)"J 4102 02 5991 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4121 02 6067 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4103 04 5995 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 4104 04 5999 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 4105 04 6003 1E");
             SV_GameSendServerCommand(i, 0, (char*)"J 4106 02 6007 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4107 04 6011 0F");
             SV_GameSendServerCommand(i, 0, (char*)"J 4108 04 6015 32");
             SV_GameSendServerCommand(i, 0, (char*)"J 4109 02 6019 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4110 02 6023 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4111 03 6027 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 4112 03 6031 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 4113 03 6035 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 4114 02 6039 O");
             SV_GameSendServerCommand(i, 0, (char*)"J 4115 03 6043 0A");
             SV_GameSendServerCommand(i, 0, (char*)"J 4116 05 6047 FA");
             SV_GameSendServerCommand(i, 0, (char*)"J 4117 05 6051 64");
             SV_GameSendServerCommand(i, 0, (char*)"J 4118 05 6055 E803");
             SV_GameSendServerCommand(i, 0, (char*)"J 4119 05 6059 2CO");
             SV_GameSendServerCommand(i, 0, (char*)"J 4120 05 6063 2CO");
             SV_GameSendServerCommand(i, 0, (char*)"J 6525 40");
		}
	}
	void DrawCrosshairOverlay() noexcept
	{
		if (!vars::enableCrosshair)
			return;
		if (!ImGui::GetCurrentContext())
			return;

		ImGuiIO& io = ImGui::GetIO();
		ImDrawList* dl = ImGui::GetBackgroundDrawList();
		const float cx = io.DisplaySize.x * 0.5f;
		const float cy = io.DisplaySize.y * 0.5f;

		const float scale = (io.DisplaySize.y / 480.f) * vars::crosshairScale;
		const float th = vars::crosshairThickness * scale;
		const float gap = vars::crosshairGap * scale * vars::crosshairGapScale;
		const float len = vars::crosshairLength * scale * vars::crosshairLengthScale;
		const float olTh = vars::crosshairOutlineThickness * scale;
		const float inner = gap;
		const float outer = gap + len;

		auto drawArms = [&](ImU32 c, float t) {
			dl->AddLine(ImVec2(cx - outer, cy), ImVec2(cx - inner, cy), c, t);
			dl->AddLine(ImVec2(cx + inner, cy), ImVec2(cx + outer, cy), c, t);
			dl->AddLine(ImVec2(cx, cy + inner), ImVec2(cx, cy + outer), c, t);
			if (!vars::crosshairTStyle)
				dl->AddLine(ImVec2(cx, cy - outer), ImVec2(cx, cy - inner), c, t);
		};

		if (vars::crosshairOutline)
			drawArms(IM_COL32(0, 0, 0, 255), th + olTh);

		drawArms(ImGui::GetColorU32(vars::crosshair_color), th);

		if (vars::crosshairCenterDot)
		{
			float dotR = th * 0.5f;
			if (dotR < 1.f) dotR = 1.f;
			if (vars::crosshairOutline)
				dl->AddCircleFilled(ImVec2(cx, cy), dotR + olTh * 0.5f, IM_COL32(0, 0, 0, 255));
			dl->AddCircleFilled(ImVec2(cx, cy), dotR, ImGui::GetColorU32(vars::crosshair_color));
		}
	}

	std::string readGameString(std::uintptr_t address, int maxLength)
	{
		void* const ptr = reinterpret_cast<void*>(address);
		DWORD origProtect = 0;
		VirtualProtect(ptr, maxLength, PAGE_EXECUTE_READWRITE, &origProtect);
		const char* charPtr = static_cast<const char*>(ptr);
		std::string result;
		for (int i = 0; i < maxLength; ++i)
		{
			if (charPtr[i] == '\0')
				break;
			result += charPtr[i];
		}
		VirtualProtect(ptr, maxLength, origProtect, &origProtect);
		return result;
	}

	int getLocalClientNum() noexcept
	{
		const std::string self = readGameString(iw4::offsets::clientName);
		if (self.empty())
			return 0;

		for (int i = 0; i < iw4::maxScoreboardClients; ++i)
		{
			if (_stricmp(self.c_str(), getPlayerName(i)) == 0)
				return i;
		}

		return 0;
	}

	static int copyScoreboardEntriesImpl(iw4::score_t* out, int maxOut) noexcept
	{
		int written = 0;

		__try
		{
			const int count = *reinterpret_cast<const int*>(iw4::offsets::cg_scoreboardPlayerCount);
			if (count <= 0)
				return 0;

			int limit = count;
			if (limit > iw4::maxScoreboardClients)
				limit = iw4::maxScoreboardClients;

			const auto* entries = reinterpret_cast<const iw4::score_t*>(iw4::offsets::cg_scoreboardEntries);

			for (int i = 0; i < limit && written < maxOut; ++i)
				out[written++] = entries[i];
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			return 0;
		}

		return written;
	}

	std::vector<iw4::score_t> getScoreboardEntries()
	{
		iw4::score_t buffer[iw4::maxScoreboardClients]{};
		const int n = copyScoreboardEntriesImpl(buffer, iw4::maxScoreboardClients);
		if (n <= 0)
			return {};

		return std::vector<iw4::score_t>(buffer, buffer + n);
	}

	bool isInGameNotSpectating()
	{
		const std::int32_t flags = *reinterpret_cast<std::int32_t*>(iw4::offsets::CG_OTHER_FLAGS);
		return (flags & iw4::offsets::OTHER_FLAG_ISINGAME) && !(flags & iw4::offsets::OTHER_FLAG_SPECTATING);
	}

}
