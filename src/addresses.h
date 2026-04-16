#pragma once
#include <cstdint>

// Centralized memory addresses for MW2 (IW4) v1.4.211
// Grouped by subsystem. Prefer these over scattered magic numbers.
namespace addr
{
    // ---- Dvar layout ----
    // All dvar pointer tables store a pointer; the actual value lives at + DVAR_VALUE_OFFSET.
    constexpr uintptr_t DVAR_VALUE_OFFSET   = 0xC;

    // ---- Per-client / stride constants ----
    constexpr int       MAX_CLIENTS         = 18;
    constexpr uintptr_t CLIENT_STRIDE       = 0x366C;   // size of client_s struct
    constexpr uintptr_t PLAYER_NAME_STRIDE  = 0x52C;
    constexpr uintptr_t PLAYER_NAME_BASE    = 0x99786C;

    // ---- Dvar pointer tables (read pointer, add DVAR_VALUE_OFFSET) ----
    constexpr uintptr_t FOV                 = 0xAAC1F8;
    constexpr uintptr_t FOV_MIN             = 0x88CB54;
    constexpr uintptr_t CHAT                = 0xAA61C0;
    constexpr uintptr_t MAP_SIZE            = 0x886E7C;
    constexpr uintptr_t BYPASS_MOUSE_INPUT  = 0xAB2B0C;
    constexpr uintptr_t MOUSE_ACCEL         = 0xBC37E0;
    constexpr uintptr_t YAW_SPEED           = 0xAB2B08;
    constexpr uintptr_t PITCH_SPEED         = 0xAB2B14;
    constexpr uintptr_t MOUSE_FILTER        = 0xBC4490;
    constexpr uintptr_t NO_CAMO_1           = 0x695D9C4;
    constexpr uintptr_t NO_CAMO_2           = 0x695D860;
    constexpr uintptr_t NO_FOG_1            = 0x695DB18;
    constexpr uintptr_t NO_FOG_2            = 0x695D9D0;
    constexpr uintptr_t NO_BULLETS          = 0x88E20C;
    constexpr uintptr_t MOVIE               = 0x695D898;
    constexpr uintptr_t CG_GUN_X            = 0xAAF7FC;
    constexpr uintptr_t CG_GUN_Y            = 0xAAF7D4;
    constexpr uintptr_t CG_GUN_Z            = 0xAAF7E4;
    constexpr uintptr_t CUSTOM_PORT         = 0x642D6CC;

    // ---- Direct memory addresses (no dvar indirection) ----
    constexpr uintptr_t FPS                 = 0x638152C;
    constexpr uintptr_t CURSOR_CAPTURE_FLAG = 0x6427D3D;
    constexpr uintptr_t HOST_ID             = 0xAB5DDC;
    constexpr uintptr_t AMMO_BASE           = 0x1B0E42C;  // + client * CLIENT_STRIDE
    constexpr uintptr_t NOCLIP_FLAG_BASE    = 0x1B114D4;  // + hostId * CLIENT_STRIDE
    constexpr uintptr_t FFA_TEAM_BASE       = 0x1B1139C - 0x80; // + client * CLIENT_STRIDE
    constexpr uintptr_t DLC_MODE            = 0x637A7C0;
    constexpr uintptr_t SENSITIVITY         = 0x063832DC;
    constexpr uintptr_t F2_CONSOLE_CMD      = 0x00AB2D88;

    // ---- Viewangles (raw input) ----
    constexpr uintptr_t VIEWANGLE_PITCH     = 0x00B343D0;
    constexpr uintptr_t VIEWANGLE_YAW       = 0x00B343D4;

    // ---- Rank / Prestige / XP ----
    constexpr uintptr_t XP                  = 0x1B8B768;
    constexpr uintptr_t PRESTIGE            = 0x1B8B770;
    constexpr uintptr_t UNLOCK_ALL_REGION   = 0x01B8BD8F;
    constexpr size_t    UNLOCK_ALL_SIZE     = 2572;

    // ---- Barracks stats ----
    constexpr uintptr_t BARRACKS_WINS         = 0x1B8B7B0;
    constexpr uintptr_t BARRACKS_LOSSES       = 0x1B8B7B4;
    constexpr uintptr_t BARRACKS_TIES         = 0x1B8B7B8;
    constexpr uintptr_t BARRACKS_WINSTREAK    = 0x1B8B7BC;
    constexpr uintptr_t BARRACKS_KILLS        = 0x1B8B77C;
    constexpr uintptr_t BARRACKS_HEADSHOTS    = 0x1B8B790;
    constexpr uintptr_t BARRACKS_ASSISTS      = 0x1B8B78C;
    constexpr uintptr_t BARRACKS_KILLSTREAK   = 0x1B8B780;
    constexpr uintptr_t BARRACKS_DEATHS       = 0x01B8B784;
    constexpr uintptr_t BARRACKS_TIMEPLAYED   = 0x1B8B79C;

    // ---- Class loadout (deagle hack) ----
    constexpr uintptr_t CLASS_2_WEAPON        = 0x1B8BB7C;
    constexpr uintptr_t CLASS_3_WEAPON        = 0x1B8BBBC;
    constexpr uintptr_t CLASS_4_WEAPON        = 0x1B8BBFC;
    constexpr uintptr_t CLASS_6_WEAPON        = 0x1B8BC7C;
    constexpr uintptr_t CLASS_8_WEAPON        = 0x1B8BCFC;
    constexpr uintptr_t CLASS_9_WEAPON        = 0x1B8BD3C;
    constexpr int       WEAPON_ID_DEAGLE      = 327776;

    // ---- Iron sight patch ----
    constexpr uintptr_t IRON_SIGHT_SCALE_1    = 0x06399684;
    constexpr uintptr_t IRON_SIGHT_SCALE_2    = 0x0639971C;
    constexpr uintptr_t IRON_SIGHT_SCALE_3    = 0x0639C43C;
    constexpr uintptr_t IRON_SIGHT_PATCH_1    = 0x2516392D;
    constexpr uintptr_t IRON_SIGHT_PATCH_2    = 0x251639AD;
    constexpr uintptr_t IRON_SIGHT_PATCH_3    = 0x2516396D;
    constexpr uintptr_t IRON_SIGHT_PATCH_4    = 0x251639ED;
    constexpr uintptr_t IRON_SIGHT_PATCH_5    = 0x25162F1F;
    constexpr uintptr_t IRON_SIGHT_LABEL      = 0x33CB8FEC;

    // ---- Byte patch sites ----
    constexpr uintptr_t PATCH_WM_MOUSEMOVE    = 0x005CCF24;
    constexpr uintptr_t PATCH_ELEVATORS       = 0x00471329;
    constexpr uintptr_t PATCH_BOUNCES         = 0x004736E2;

    // ---- Menu UI text ----
    constexpr uintptr_t UI_ITNET              = 0x33BD7AF4;
    constexpr uintptr_t UI_FIND_GAME          = 0x33BD80C0;
    constexpr uintptr_t UI_PRIVATE_MATCH      = 0x33BD8519;
    constexpr uintptr_t UI_CREATE_CLASS       = 0x33BD8A99;
    constexpr uintptr_t UI_KILLSTREAKS        = 0x33BD9BE0;
    constexpr uintptr_t UI_BARRACKS           = 0x33BDA69E;
    constexpr uintptr_t UI_INVITE             = 0x33BDAFE1;
    constexpr uintptr_t UI_PLAYLISTS          = 0x33BE3F80;
    constexpr uintptr_t UI_START_GAME         = 0x33C0759C;
    constexpr uintptr_t UI_SETTINGS           = 0x33C079A5;
    constexpr uintptr_t UI_TITLE              = 0x33CBD9E6;
}