#include "variables.h"


bool variables::debug = false;

int variables::iFPS = 400;
float variables::fFieldOfView = 90.0f;
float variables::fMapSize = 1.0f;
bool variables::bChat = true;
float variables::fFOVMinDefault = 1.0f; // default FOV min
bool variables::fFOVMin = false;        // Enable Mouse 1:1 toggle
float variables::fFOVMinSlider = 90.0f; // Mouse 1:1 slider value
int variables::fMouseFilter = 0;
int variables::fNoCamo1 = 0;
int variables::fNoCamo2 = 0;
bool variables::fNoCamoBool = false;
int variables::fNoFog1 = 0;
int variables::fNoFog2 = 0;
bool variables::fNoCFogBool = false;
int variables::fNoBullets = 0;
bool variables::fNoBulletsBool = false;
bool variables::bDrawSun = true;
bool variables::bGlass = true;
int variables::iFullbright = 1;
int variables::iLightMap = 0;
int variables::fMovie = 1;
bool variables::fMovieBool = false;
float variables::fSens = 1.0f;

bool variables::bMouseFix = true;
int variables::fBypassMouseInput = 0;
float variables::fMouseAccel = 0.0f;
float variables::fYawSpeed = 0.0f;
float variables::fPitchSpeed = 0.0f;

bool variables::bEnableDLC = false;
int variables::iDLCMode = 0;

int variables::iPrestige = 10;
char variables::BarracksWins[128] = "1000";
char variables::BarracksLosses[128] = "250";
char variables::BarracksTies[128] = "10";
char variables::BarracksWinStreak[128] = "22";
char variables::BarracksKills[128] = "30000";
char variables::BarracksHeadshots[128] = "6000";
char variables::BarracksAssists[128] = "1000";
char variables::BarracksKillStreak[128] = "58";
char variables::BarracksDeaths[128] = "14000";
char variables::BarracksTimePlayed[128] = "99999";

char variables::Console[500] = "r_fullbright 1";
char variables::ServerCommand[500000] = "J 2064 0A000";
char variables::MenuName[500] = "popup_gamesetup";

// Host

const char *variables::realmaplist[26] = {"mp_afghan", "mp_derail", "mp_estate", "mp_favela", "mp_highrise",
                                          "mp_invasion", "mp_checkpoint", "mp_quarry", "mp_rundown", "mp_rust", "mp_boneyard", "mp_nightshift", "mp_subbase",
                                          "mp_terminal", "mp_underpass", "mp_brecourt", "mp_complex", "mp_crash", "mp_compact", "mp_overgrown", "mp_storm",
                                          "mp_abandon", "mp_fuel2", "mp_strike", "mp_trailerpark", "mp_vacant"};
const char *variables::map_list[26] = {
    "Afghan", "Derail", "Estate", "Favela", "Highrise", "Invasion", "Karachi",
    "Quarry", "Rundown", "Rust", "Scrapyard", "Skidrow", "Sub Base", "Terminal",
    "Underpass", "Wasteland", "Bailout", "Crash", "Salvage",
    "Overgrown", "Storm", "Carnival", "Fuel", "Strike", "Trailer Park", "Vacant"};
int variables::imap_list_number = 0;
const char *variables::gamemode_list[12] = {
    "Domination", "Team Deathmatch", "Search and Destroy", "Free-For-All", "Headquarters", "Demolition", "Sabotage", "Capture the Flag", "Global Thermonuclear War", "One Flag CTF", "VIP", "Arena"};
const char *variables::realgamemode_list[12] = {
    "dom", "war", "sd", "ffa", "koth", "dem", "sab", "ctf", "gtnw", "oneflag", "vip", "arena"};
int variables::igamemode_list_number = 0;
int variables::iMaxPlayers = 18;
bool variables::bFFATeamFix = false;

float variables::fcg_gun_x = 0.0f;
float variables::fcg_gun_y = 0.0f;
float variables::fcg_gun_z = 0.0f;
bool variables::bNoClip = false;

bool variables::bBounces = false;
bool variables::bElevators = false;
bool variables::bEnableNameLogging = false;

int variables::customPort = 28960;

bool variables::bClient[18] = {false};
char variables::clientName[18][15] = {""};
float variables::fSavedPlayerX = 0.0f;
float variables::fSavedPlayerY = 0.0f;
float variables::fSavedPlayerZ = 0.0f;
float variables::fSavedPlayerViewAngleY = 0.0f;
float variables::fSavedPlayerViewAngleX = 0.0f;

float variables::fPlayerX = 0.0f;
float variables::fPlayerY = 0.0f;
float variables::fPlayerZ = 0.0f;
float variables::fPlayerViewAngleY = 0.0f;
float variables::fPlayerViewAngleX = 0.0f;
bool variables::bUseInsertKey = true;
bool variables::bPingText = true;
bool variables::bIronSightInter = false;

Position variables::SavedLocationOne = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f};

// vars.cpp
#pragma region GET RID OF LONG SHIT

const char *variables::FullVisionListString =
    "default\n"
    "ac130\n"
    "ac130_inverted\n"
    "af_caves_indoors\n"
    "af_caves_indoors_breachroom\n"
    "af_caves_indoors_overlook\n"
    "af_caves_indoors_skylight\n"
    "af_caves_indoors_steamroom\n"
    "af_caves_indoors_steamroom_dark\n"
    "af_caves_outdoors\n"
    "af_caves_outdoors_airstrip\n"
    "af_chase_ending_noshock\n"
    "af_chase_indoors\n"
    "af_chase_indoors_2\n"
    "af_chase_indoors_3\n"
    "af_chase_outdoors\n"
    "af_chase_outdoors_2\n"
    "af_chase_outdoors_3\n"
    "aftermath\n"
    "aftermath_dying\n"
    "aftermath_hurt\n"
    "aftermath_nodesat\n"
    "aftermath_pain\n"
    "aftermath_walking\n"
    "airplane\n"
    "airport\n"
    "airport_death\n"
    "airport_exterior\n"
    "airport_green\n"
    "airport_intro\n"
    "airport_stairs\n"
    "ambush\n"
    "arcadia\n"
    "arcadia_checkpoint\n"
    "arcadia_golfcourse\n"
    "arcadia_house2\n"
    "arcadia_house\n"
    "arcadia_secondbridge\n"
    "arcadia_secondstreet\n"
    "arcadia_wrekage\n"
    "armada\n"
    "armada_ground\n"
    "armada_sound\n"
    "armada_tvs\n"
    "armada_water\n"
    "bigcity_destroyed\n"
    "black_bw\n"
    "blackout\n"
    "blackout_darkness\n"
    "blackout_nvg\n"
    "blacktest\n"
    "bog\n"
    "bog_a\n"
    "bog_a_sunrise\n"
    "bog_b\n"
    "boneyard\n"
    "boneyard_flyby\n"
    "boneyard_ride\n"
    "boneyard_steer\n"
    "bridge\n"
    "cargoship\n"
    "cargoship_blast\n"
    "cargoship_indoor2\n"
    "cargoship_indoor\n"
    "cheat_bw\n"
    "cheat_bw_contrast\n"
    "cheat_bw_invert\n"
    "cheat_bw_invert_contrast\n"
    "cheat_chaplinnight\n"
    "cheat_contrast\n"
    "cheat_invert\n"
    "cheat_invert_contrast\n"
    "cliffhanger\n"
    "cliffhanger_extreme\n"
    "cliffhanger_heavy\n"
    "cliffhanger_snowmobile\n"
    "co_break\n"
    "co_overgrown\n"
    "cobra_down\n"
    "cobra_sunset1\n"
    "cobra_sunset2\n"
    "cobra_sunset3\n"
    "cobrapilot\n"
    "contingency\n"
    "contingency_thermal_inverted\n"
    "coup\n"
    "coup_hit\n"
    "coup_sunblind\n"
    "dc_whitehouse_interior\n"
    "dc_whitehouse_lawn\n"
    "dc_whitehouse_roof\n"
    "dc_whitehouse_tunnel\n"
    "dcburning_bunker\n"
    "dcburning_commerce\n"
    "dcburning_crash\n"
    "dcburning_heliride\n"
    "dcburning_rooftops\n"
    "dcburning_trnches\n"
    "dcemp\n"
    "dcemp_emp\n"
    "dcemp_iss\n"
    "dcemp_iss\n"
    "dcemp_iss_death\n"
    "dcemp_office\n"
    "dcemp_parking\n"
    "dcemp_parking_lighting\n"
    "dcemp_postemp2\n"
    "dcemp_postemp\n"
    "dcemp_tunnels\n"
    "default_night\n"
    "default_night_mp\n"
    "downtown_la\n"
    "end_game2\n"
    "end_game\n"
    "estate\n"
    "estate_ambush_clearing\n"
    "estate_birchfield\n"
    "estate_dragplayer\n"
    "estate_finalfield\n"
    "estate_forest\n"
    "estate_forset_clearing\n"
    "estate_hilltop\n"
    "estate_house_approach\n"
    "estate_house_backyard\n"
    "estate_house_interior\n"
    "estate_throwplayer\n"
    "exterior_concept\n"
    "favela\n"
    "favela_alleys\n"
    "favela_chase\n"
    "favela_ending\n"
    "favela_escape\n"
    "favela_escape_chopperjump\n"
    "favela_escape_market\n"
    "favela_escape_playerfall_recovery\n"
    "favela_escape_radiotower\n"
    "favela_escape_rooftops\n"
    "favela_escape_soccerfield\n"
    "favela_escape_soccerfield_buildings\n"
    "favela_escape_solorun_buildings\n"
    "favela_escape_solorun_nearend\n"
    "favela_escape_street\n"
    "favela_hill\n"
    "favela_shanty\n"
    "favela_torture\n"
    "firingrange\n"
    "grayscale\n"
    "gulag\n"
    "gulag_circle\n"
    "gulag_ending\n"
    "gulag_flyin\n"
    "gulag_hallways\n"
    "gulag_nvg\n"
    "helicopter_ride\n"
    "hunted\n"
    "hunted_crash\n"
    "icbm\n"
    "icbm_interior\n"
    "icbm_launch\n"
    "icbm_sunrise0\n"
    "icbm_sunrise1\n"
    "icbm_sunrise2\n"
    "icbm_sunrise3\n"
    "icbm_sunrise4\n"
    "interior_concept\n"
    "introscreen\n"
    "invasion\n"
    "invasion_alley\n"
    "invasion_nates_roof\n"
    "invasion_near_convoy\n"
    "invasion_stripmall\n"
    "invasion_suburban_streets\n"
    "invasion_yards\n"
    "jeepride\n"
    "jeepride_cobra\n"
    "jeepride_flyaway\n"
    "jeepride_tunnel\n"
    "jeepride_zak\n"
    "killhouse\n"
    "launchfacility\n"
    "launchfacility_a\n"
    "launchfacility_b\n"
    "missilecam\n"
    "mp_backlot\n"
    "mp_bloc\n"
    "mp_bog\n"
    "mp_brecourt\n"
    "mp_broadcase\n"
    "mp_carentan\n"
    "mp_cargoship\n"
    "mp_citystreets\n"
    "mp_convoy\n"
    "mp_countdown\n"
    "mp_crash\n"
    "mp_crash_damage\n"
    "mp_creek\n"
    "mp_creek_ss\n"
    "mp_crossfire\n"
    "mp_derail\n"
    "mp_downtown_la\n"
    "mp_dusk\n"
    "mp_farm\n"
    "mp_favela\n"
    "mp_firingrange\n"
    "mp_highrise\n"
    "mp_hill\n"
    "mp_killhouse\n"
    "mp_nightshift\n"
    "mp_oilrig\n"
    "mp_overgrown\n"
    "mp_pipeline\n"
    "mp_quarry\n"
    "mp_riverwalk\n"
    "mp_shipment\n"
    "mp_showdown\n"
    "mp_skidrow\n"
    "mp_strike\n"
    "mp_suburbia\n"
    "mp_trailer\n"
    "mp_vacant\n"
    "mp_verdict\n"
    "mpintro\n"
    "mpnuke\n"
    "mpnuke_aftermath\n"
    "mpoutro\n"
    "nate_test\n"
    "near_death\n"
    "near_death_mp\n"
    "oilrig_exterior_deck0\n"
    "oilrig_exterior_deck1\n"
    "oilrig_exterior_deck2\n"
    "oilrig_exterior_deck3\n"
    "oilrig_exterior_deck4\n"
    "oilrig_exterior_heli\n"
    "oilrig_interior2\n"
    "oilrig_interior\n"
    "oilrig_underwater\n"
    "overwatch\n"
    "overwatch_nv\n"
    "parabolic\n"
    "roadkill\n"
    "roadkill_ambush\n"
    "roadkill_dismount_building\n"
    "roadkill_ending\n"
    "roadkill_inside_school\n"
    "roadkill_left_school\n"
    "roadkill_town_normal\n"
    "roadkill_town_smokey\n"
    "roadkill_walking_to_school\n"
    "school\n"
    "scoutsniper\n"
    "seaknight_assault\n"
    "sepia\n"
    "slomo_breach\n"
    "sniperescape\n"
    "sniperescape_glow_off\n"
    "sniperescape_outside\n"
    "so_bridge\n"
    "strike\n"
    "thermal_mp\n"
    "trainer_pit\n"
    "trainer_start\n"
    "tulsa\n"
    "village_assauilt\n"
    "village_defend\n"
    "wetwork\n"
    "whitehouse";

#pragma endregion
