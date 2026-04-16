#pragma once

struct Position {
    float x, y, z, viewx, viewy;
};

class variables
{
public:
    static bool debug;
    static int iFPS;
    static float fFieldOfView;
    static bool bEnableDLC;
    static int iDLCMode;
    static float fMapSize;
    static bool bChat;
    static float fFOVMinDefault;
    static bool fFOVMin;
    static float fFOVMinSlider;
    static int fNoCamo1;
    static int fNoCamo2;
    static bool fNoCamoBool;
    static int fNoFog1;
    static int fNoFog2;
    static bool fNoCFogBool;
    static int fNoBullets;
    static bool fNoBulletsBool;
    static bool bDrawSun;
    static bool bGlass;
    static int iFullbright;
    static int iLightMap;
    static int fMovie;
    static bool fMovieBool;
    static bool bMouseFix;
    static int fBypassMouseInput;
    static float fMouseAccel;
    static float fYawSpeed;
    static float fPitchSpeed;
    static int fMouseFilter;
    static int iPrestige;
    static char BarracksWins[128];
    static char BarracksLosses[128];
    static char BarracksTies[128];
    static char BarracksWinStreak[128];
    static char BarracksKills[128];
    static char BarracksHeadshots[128];
    static char BarracksAssists[128];
    static char BarracksKillStreak[128];
    static char BarracksDeaths[128];
    static char BarracksTimePlayed[128];
    static const char *map_list[26];
    static const char *realmaplist[26];
    static int imap_list_number;
    static const char *gamemode_list[12];
    static const char *realgamemode_list[12];
    static int igamemode_list_number;
    static int iMaxPlayers;
    static bool bFFATeamFix;
    static float fcg_gun_x;
    static float fcg_gun_y;
    static float fcg_gun_z;
    static char Console[500];
    static char ServerCommand[500000];
    static char MenuName[500];
    static bool bBounces;
    static bool bElevators;
    static bool bEnableNameLogging;
    static int customPort;
    static bool bNoClip;
    static const char *FullVisionListString;
    static bool bClient[18];
    static char clientName[18][15];
    static float fSavedPlayerX;
    static float fSavedPlayerY;
    static float fSavedPlayerZ;
    static float fSavedPlayerViewAngleY;
    static float fSavedPlayerViewAngleX;
    static float fPlayerX;
    static float fPlayerY;
    static float fPlayerZ;
    static float fPlayerViewAngleY;
    static float fPlayerViewAngleX;
    static bool bUseInsertKey;
    static float fSens;
    static bool bPingText;
    static bool bIronSightInter;


    // This now works because Position is defined in engine_hooks.h
    static Position SavedLocationOne;
};
