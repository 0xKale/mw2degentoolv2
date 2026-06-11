//! Game-logic core. Port of `src/dismay/functions.cpp`.
//!
//! Everything here pokes the live MW2 process: resolving dvar pointers, writing
//! absolute addresses, patching code bytes, and calling engine functions. The
//! two ImGui-dependent routines from the original (`syncImGuiMouseDrawCursor`,
//! `DrawCrosshairOverlay`) live in the rendering layer instead.

#![allow(non_snake_case, dead_code)]

use crate::game::funcs;
use crate::game::offsets as off;
use crate::game::offsets::dvar as dv;
use crate::game::structs::{score_t, DLCDef, DLCList, MAX_SCOREBOARD_CLIENTS};
use crate::gui_state;
use crate::state::{vars, MAP_LIST};
use crate::win32;
use core::ffi::c_void;
use core::sync::atomic::{AtomicBool, Ordering};
use std::sync::Mutex;
use std::thread::JoinHandle;

/// Offset from a resolved `dvar_t*` to its value (`iw4::pointers::Dvar`).
const DVAR: usize = off::pointers::DVAR;

// --- raw memory helpers -----------------------------------------------------

#[inline]
unsafe fn rd<T: Copy>(addr: usize) -> T {
    (addr as *const T).read_volatile()
}
#[inline]
unsafe fn wr<T>(addr: usize, value: T) {
    (addr as *mut T).write_volatile(value)
}

/// `Cbuf_AddText(0, text)` for a NUL-terminated literal.
#[inline]
fn cbuf(text: &[u8]) {
    debug_assert_eq!(*text.last().unwrap(), 0, "cbuf text must be NUL-terminated");
    unsafe { funcs::Cbuf_AddText(0, text.as_ptr()) }
}

/// `Cbuf_AddText(0, text)` for a runtime-built string.
#[inline]
fn cbuf_str(s: &str) {
    let mut v: Vec<u8> = Vec::with_capacity(s.len() + 1);
    v.extend_from_slice(s.as_bytes());
    v.push(0);
    unsafe { funcs::Cbuf_AddText(0, v.as_ptr()) }
}

/// Scan a NUL-terminated C string (bounded) for a substring.
unsafe fn cstr_contains(haystack: *const u8, needle: &[u8]) -> bool {
    if haystack.is_null() || needle.is_empty() {
        return false;
    }
    let mut len = 0usize;
    while len < 256 && *haystack.add(len) != 0 {
        len += 1;
    }
    let hay = core::slice::from_raw_parts(haystack, len);
    hay.windows(needle.len()).any(|w| w == needle)
}

/// `strcpy_s(addr, cap, s)` into raw game memory (no VirtualProtect, matching
/// the original UI-string writes).
unsafe fn strcpy_bounded(addr: usize, cap: usize, s: &[u8]) {
    let n = s.len().min(cap.saturating_sub(1));
    core::ptr::copy_nonoverlapping(s.as_ptr(), addr as *mut u8, n);
    *((addr + n) as *mut u8) = 0;
}

// --- dvars ------------------------------------------------------------------

pub fn SetDvarInt(dvar_address: usize, value: i32) {
    unsafe {
        let p: u32 = rd(dvar_address);
        wr::<i32>(p as usize + DVAR, value);
    }
}
pub fn SetDvarFloat(dvar_address: usize, value: f32) {
    unsafe {
        let p: u32 = rd(dvar_address);
        wr::<f32>(p as usize + DVAR, value);
    }
}
fn ReadDvarInt(dvar_address: usize) -> i32 {
    unsafe {
        let p: u32 = rd(dvar_address);
        if p == 0 {
            return 0;
        }
        rd::<i32>(p as usize + DVAR)
    }
}
fn ReadDvarFloat(dvar_address: usize) -> f32 {
    unsafe {
        let p: u32 = rd(dvar_address);
        if p == 0 {
            return 0.0;
        }
        rd::<f32>(p as usize + DVAR)
    }
}

pub fn getHostId() -> i32 {
    unsafe { rd::<i32>(off::hostId) }
}

// --- visual / dvar toggles --------------------------------------------------

pub fn fuckTheSunAway() {
    SetDvarInt(dv::r_drawSun, if vars().noSun { 1 } else { 0 });
}

pub fn fuckTheCrosshairAway() {
    // Custom crosshair on => hide engine crosshair.
    let want = if vars().enableCrosshair { 0 } else { 1 };
    if want == ReadDvarInt(dv::cg_drawCrosshair) {
        return;
    }
    unsafe {
        let p: u32 = rd(dv::cg_drawCrosshair);
        if p == 0 {
            return;
        }
    }
    SetDvarInt(dv::cg_drawCrosshair, want);
}

pub fn clearGlass() {
    if !vars().clearGlass {
        cbuf(b"glass_angular_vel 5 35;glass_edge_angle 5 10;glass_fall_delay 0.2 0.9;glass_fall_gravity 800;glass_fall_ratio 1.5 3;glass_fringe_maxcoverage 0.2;glass_fringe_maxsize 150;glass_fx_chance 0.25;glass_hinge_friction 50;glass_linear_vel 200 400;glass_max_pieces_per_frame 100;glass_max_shatter_fx_per_frame 6;glass_physics_chance 0.15;glass_physics_maxdist 512;glass_shard_maxsize 300;glass_shattered_scale 48;glass_trace_interval 100\0");
    } else {
        cbuf(b"glass_angular_vel 180 180;glass_edge_angle 5 10;glass_fall_delay 0 0;glass_fall_gravity 800;glass_fall_ratio 0 0;glass_fringe_maxcoverage 0;glass_fringe_maxsize 0;glass_fx_chance 0;glass_hinge_friction 0;glass_linear_vel 10000 10000;glass_max_pieces_per_frame 1;glass_max_shatter_fx_per_frame 1;glass_physics_chance 0;glass_physics_maxdist 0;glass_shard_maxsize 1;glass_shattered_scale 999999;glass_trace_interval 1\0");
    }
}

pub fn sendNoCamo() {
    if vars().drawCamo {
        SetDvarInt(dv::r_detail, 1);
        SetDvarInt(dv::r_detailMap, 1);
    } else {
        SetDvarInt(dv::r_detail, 0);
        SetDvarInt(dv::r_detailMap, 0);
    }
}

pub fn sendNoFog() {
    if vars().drawFog {
        SetDvarInt(dv::r_fog, 1);
        SetDvarInt(dv::fx_drawClouds, 1);
    } else {
        SetDvarInt(dv::r_fog, 0);
        SetDvarInt(dv::fx_drawClouds, 0);
    }
}

pub fn sendNoBullets() {
    if vars().drawBullets {
        SetDvarInt(dv::cg_brass, 1);
        SetDvarInt(dv::fx_marks, 1);
    } else {
        SetDvarInt(dv::cg_brass, 0);
        SetDvarInt(dv::fx_marks, 0);
    }
}

pub fn sendMovie() {
    SetDvarInt(dv::r_filmUseTweaks, if vars().movieMode { 1 } else { 0 });
}

pub fn sendFPSandFOV() {
    let v = vars();
    if v.framesPerSecond != ReadDvarInt(dv::com_maxFPS) {
        SetDvarInt(dv::com_maxFPS, v.framesPerSecond);
    }
    if v.fieldOfView != ReadDvarFloat(dv::cg_fov) {
        SetDvarFloat(dv::cg_fov, v.fieldOfView);
    }
}

pub fn sendMapSize() {
    SetDvarFloat(dv::compassSize, vars().mapSize);
}

fn isOneToOneExcludedWeapon() -> bool {
    unsafe {
        let weapon = funcs::get_current_weapon_name();
        if weapon.is_null() {
            return false;
        }
        cstr_contains(weapon, b"cheytac")
            || cstr_contains(weapon, b"barrett")
            || cstr_contains(weapon, b"wa2000")
            || cstr_contains(weapon, b"m21")
    }
}

fn isCheytacAcogWeapon() -> bool {
    unsafe {
        let weapon = funcs::get_current_weapon_name();
        if weapon.is_null() {
            return false;
        }
        cstr_contains(weapon, b"cheytac") && cstr_contains(weapon, b"acog")
    }
}

pub fn sendFOVMin() {
    let v = vars();
    if v.enableMouseOneToOne && !isOneToOneExcludedWeapon() {
        if v.fieldOfView != ReadDvarFloat(dv::cg_fovMin) {
            SetDvarFloat(dv::cg_fovMin, v.fieldOfView);
        }
    } else {
        SetDvarFloat(dv::cg_fovMin, v.defaultFovMin);
    }
}

pub fn toggleChat() {
    SetDvarInt(
        dv::cg_chatTime,
        if vars().enableTextChat { 12000 } else { 0 },
    );
}

pub fn mouseFix() {
    SetDvarInt(dv::cl_bypassMouseInput, 0);
    SetDvarFloat(dv::cl_mouseAccel, 0.0);
    SetDvarInt(dv::m_filter, 0);
}

pub fn NetworkFix() {
    let packets = 100;
    if ReadDvarInt(dv::cl_maxpackets) != packets {
        SetDvarInt(dv::cl_maxpackets, packets);
    }
    let packetdup = 5;
    if ReadDvarInt(dv::cl_packetdup) != packetdup {
        SetDvarInt(dv::cl_packetdup, packetdup);
    }
}

// --- barracks / profile -----------------------------------------------------

pub fn doSaveBarracks() {
    let v = vars();
    unsafe {
        wr::<i32>(off::BarracksWins, v.wins);
        wr::<i32>(off::BarracksLosses, v.losses);
        wr::<i32>(off::BarracksTies, v.ties);
        wr::<i32>(off::BarracksWinStreak, v.winStreak);
        wr::<i32>(off::BarracksKills, v.kills);
        wr::<i32>(off::BarracksHeadshots, v.headshots);
        wr::<i32>(off::BarracksAssists, v.assists);
        wr::<i32>(off::BarracksKillStreak, v.killStreak);
        wr::<i32>(off::BarracksDeaths, v.deaths);
        wr::<i32>(off::BarracksTimePlayed, v.timePlayed);
    }
}

pub fn loadProfileStats() {
    let v = vars();
    unsafe {
        v.wins = rd::<i32>(off::BarracksWins);
        v.losses = rd::<i32>(off::BarracksLosses);
        v.ties = rd::<i32>(off::BarracksTies);
        v.winStreak = rd::<i32>(off::BarracksWinStreak);
        v.kills = rd::<i32>(off::BarracksKills);
        v.headshots = rd::<i32>(off::BarracksHeadshots);
        v.assists = rd::<i32>(off::BarracksAssists);
        v.killStreak = rd::<i32>(off::BarracksKillStreak);
        v.deaths = rd::<i32>(off::BarracksDeaths);
        v.timePlayed = rd::<i32>(off::BarracksTimePlayed);
    }
}

pub fn sendGoldDeagleClasses() {
    unsafe {
        wr::<i32>(0x1B8BB7C, 327776); // class 2
        wr::<i32>(0x1B8BBBC, 327776); // class 3
        wr::<i32>(0x1B8BBFC, 327776); // class 4
        wr::<i32>(0x1B8BC7C, 327776); // class 6
        wr::<i32>(0x1B8BCFC, 327776); // class 8
        wr::<i32>(0x1B8BD3C, 327776); // class 9
    }
}

// --- raw memory write primitives -------------------------------------------

pub fn writeMemory(addr: usize, bytes: &[u8]) {
    unsafe {
        let mut old: u32 = 0;
        win32::VirtualProtect(
            addr as *mut c_void,
            bytes.len(),
            win32::PAGE_EXECUTE_READWRITE,
            &mut old,
        );
        core::ptr::copy_nonoverlapping(bytes.as_ptr(), addr as *mut u8, bytes.len());
        win32::VirtualProtect(addr as *mut c_void, bytes.len(), old, &mut old);
    }
}

/// `WriteBytes` — like [`writeMemory`] but does *not* restore the old
/// protection (faithful to the original).
pub fn WriteBytes(addr: usize, bytes: &[u8]) {
    unsafe {
        let mut old: u32 = 0;
        win32::VirtualProtect(
            addr as *mut c_void,
            bytes.len(),
            win32::PAGE_EXECUTE_READWRITE,
            &mut old,
        );
        core::ptr::copy_nonoverlapping(bytes.as_ptr(), addr as *mut u8, bytes.len());
    }
}

pub fn unlockAll() {
    let buf = [0x90u8; 2572];
    writeMemory(0x01B8BD8F, &buf);
}

// --- rank / xp --------------------------------------------------------------

fn xpToAdvanceFromRank(n: i32) -> i32 {
    // XP to go from rank n -> n+1 (MW2 table / piecewise segments).
    if n < 1 || n > 70 {
        return 0;
    }
    if n <= 10 {
        return 500 + 700 * (n - 1);
    }
    if n <= 29 {
        return 7800 + 1000 * (n - 11);
    }
    if n <= 49 {
        return 27000 + 1200 * (n - 30);
    }
    51300 + 1500 * (n - 50)
}

fn totalXpAtRank(rank: i32) -> u32 {
    if rank <= 1 {
        return 0;
    }
    let rank = if rank > 70 { 70 } else { rank };
    let mut sum: u32 = 0;
    for i in 1..rank {
        sum = sum.wrapping_add(xpToAdvanceFromRank(i) as u32);
    }
    sum
}

pub fn doLevel70() {
    unsafe { wr::<u32>(off::LocalClientLevel, totalXpAtRank(70)) }
}
pub fn doLevel1() {
    unsafe { wr::<u32>(off::LocalClientLevel, 0) }
}
pub fn sendPrestige(prestige: i32) {
    unsafe { wr::<u32>(off::LocalClientPrestige, prestige as u32) }
}
pub fn sendRank() {
    unsafe { wr::<u32>(off::LocalClientLevel, totalXpAtRank(vars().rank)) }
}

// --- DLC --------------------------------------------------------------------

pub fn doDLCMaps() {
    const ORIGINAL_MAPS: [DLCDef; 1] = [DLCDef {
        a2: 2,
        name: b"MP_ORIGINAL_MAPS",
    }];
    const ALL_MAPS: [DLCDef; 3] = [
        DLCDef {
            a2: 2,
            name: b"MP_ORIGINAL_MAPS",
        },
        DLCDef {
            a2: 4,
            name: b"DLC_1",
        },
        DLCDef {
            a2: 8,
            name: b"DLC_2",
        },
    ];

    let (items, item_count): (&[DLCDef], usize) = if vars().enableDLC {
        (&ALL_MAPS, ALL_MAPS.len())
    } else {
        (&ORIGINAL_MAPS, ORIGINAL_MAPS.len())
    };
    let max_items = ALL_MAPS.len();

    for i in 0..max_items {
        let mut item = DLCList::zeroed();
        if i < item_count {
            item.a2 = items[i].a2;
            item.flag1 = 1;
            item.flag2 = 1;
            let src = items[i].name;
            let n = src.len().min(127);
            item.name[..n].copy_from_slice(&src[..n]);
        }
        let address = off::dlc_location + i * core::mem::size_of::<DLCList>();
        let item_bytes = unsafe {
            core::slice::from_raw_parts(
                &item as *const DLCList as *const u8,
                core::mem::size_of::<DLCList>(),
            )
        };
        WriteBytes(address, item_bytes);
    }

    WriteBytes(off::dlc_count, &(item_count as i32).to_le_bytes());
}

// --- host / match -----------------------------------------------------------

pub fn doMaxPlayers(amount: i32) {
    if amount < 2 || amount > 18 {
        return;
    }
    let command =
        format!("sv_maxclients {amount};party_maxplayers {amount};ui_maxclients {amount}");
    cbuf_str(&command);
}

pub fn doStartMatch() {
    cbuf(b";xblive_privatematch 1;wait 2;xpartygo;wait 2;xblive_privatematch 0;\0");
}

pub fn doBalanceTeams() {
    unsafe {
        funcs::BalanceTeams(off::G_LOBBYDATA as *mut c_void);
        funcs::BalanceTeams(off::PARTYSESSION_P as *mut c_void);
    }
}

pub fn FastRestart() {
    unsafe { funcs::MapRestart(0, 0) }
}

pub fn ChangeMap() {
    let idx = vars().selectedMap;
    if idx < 0 || idx as usize >= MAP_LIST.len() {
        return;
    }
    let name = MAP_LIST[idx as usize];
    unsafe { funcs::SV_SpawnServer(name.as_ptr() as *mut u8, 0, 0) }
}

pub fn ChangeGamemode() {
    match vars().selectedGamemode {
        0 => cbuf(b"g_gametype dom; ui_gametype dom; party_gametype dom\0"),
        1 => cbuf(b"g_gametype war; ui_gametype war; party_gametype war\0"),
        2 => cbuf(b"g_gametype sd; ui_gametype sd; party_gametype sd\0"),
        3 => cbuf(b"g_gametype dm; ui_gametype dm; party_gametype dm\0"),
        4 => cbuf(b"g_gametype koth; ui_gametype koth; party_gametype koth\0"),
        5 => cbuf(b"g_gametype dem; ui_gametype dem; party_gametype dem\0"),
        6 => cbuf(b"g_gametype sab; ui_gametype sab; party_gametype sab\0"),
        7 => cbuf(b"g_gametype ctf; ui_gametype ctf; party_gametype ctf\0"),
        8 => cbuf(b"g_gametype gtnw; ui_gametype gtnw; party_gametype gtnw\0"),
        9 => cbuf(b"g_gametype oneflag; ui_gametype oneflag; party_gametype oneflag\0"),
        10 => cbuf(b"g_gametype vip; ui_gametype vip; party_gametype vip\0"),
        11 => cbuf(b"g_gametype arena; ui_gametype arena; party_gametype arena\0"),
        _ => cbuf(b"g_gametype dom; ui_gametype dom; party_gametype dom\0"),
    }
}

pub fn doForceHost() {
    for cmd in FORCE_HOST_CMDS {
        cbuf(cmd);
    }
}

static FORCE_HOST_CMDS: [&[u8]; 33] = [
    b"party_connectTimeout 1000\0",
    b"party_connectTimeout 1\0",
    b"party_host 1\0",
    b"party_hostmigration 0\0",
    b"onlinegame 1\0",
    b"onlinegameandhost 1\0",
    b"onlineunrankedgameandhost 0\0",
    b"migration_msgtimeout 0\0",
    b"migration_timeBetween 999999\0",
    b"migration_verboseBroadcastTime 0\0",
    b"migrationPingTime 0\0",
    b"bandwidthtest_duration 0\0",
    b"bandwidthtest_enable 0\0",
    b"bandwidthtest_ingame_enable 0\0",
    b"bandwidthtest_timeout 0\0",
    b"cl_migrationTimeout 0\0",
    b"lobby_partySearchWaitTime 0\0",
    b"bandwidthtest_announceinterval 0\0",
    b"partymigrate_broadcast_interval 99999\0",
    b"partymigrate_pingtest_timeout 0\0",
    b"partymigrate_timeout 0\0",
    b"partymigrate_timeoutmax 0\0",
    b"partymigrate_pingtest_retry 0\0",
    b"partymigrate_pingtest_timeout 0\0",
    b"g_kickHostIfIdle 0\0",
    b"sv_cheats 1\0",
    b"xblive_playEvenIfDown 1\0",
    b"party_hostmigration 0\0",
    b"badhost_endGameIfISuck 0\0",
    b"badhost_maxDoISuckFrames 0\0",
    b"badhost_maxHappyPingTime 99999\0",
    b"badhost_minTotalClientsForHappyTest 99999\0",
    b"bandwidthtest_enable 0\0",
];

pub fn doFFATeamFix() {
    if vars().FFATeamFix {
        for i in 0..18i32 {
            unsafe {
                wr::<i32>(
                    (0x1B1139Cusize - 0x80).wrapping_add((0x366C * i) as usize),
                    0,
                );
            }
        }
    }
}

static NOCLIP: AtomicBool = AtomicBool::new(false);

pub fn handleHotkeys() {
    if !vars().enableHostHotkeys {
        return;
    }
    unsafe {
        if win32::GetAsyncKeyState(win32::VK_F2) & 1 != 0 {
            funcs::Cbuf_AddText(0, 0x00AB2D88 as *const u8);
        }
        if win32::GetAsyncKeyState(win32::VK_F3) & 1 != 0 {
            doForceHost();
        }
        if win32::GetAsyncKeyState(win32::VK_F4) & 1 != 0 {
            ChangeGamemode();
            funcs::OpenMenu(0, b"popup_gamesetup\0".as_ptr());
            cbuf(b"xblive_privatematch 1\0");
        }
        if win32::GetAsyncKeyState(win32::VK_F5) & 1 != 0 {
            ChangeGamemode();
            doMaxPlayers(vars().maxPlayers);
            doStartMatch();
            doBalanceTeams();
        }
        if win32::GetAsyncKeyState(win32::VK_X) & 1 != 0 {
            let n = !NOCLIP.fetch_xor(true, Ordering::Relaxed); // new value
            let val: i32 = if n { 1 } else { 0 };
            wr::<i32>(
                0x1B114D4usize.wrapping_add((getHostId() * 0x366C) as usize),
                val,
            );
        }
    }
}

pub fn sendElevatorsToggle() {
    let depatch = [0xEBu8, 0x42];
    let original = [0x4Au8, 0x2A];
    if vars().enableDepatchElevators {
        WriteBytes(0x00471329, &depatch);
    } else {
        WriteBytes(0x00471329, &original);
    }
}

pub fn sendBouncesToggle() {
    let depatch = [0x90u8, 0x90];
    let original = [0x75u8, 0x14];
    if vars().enableDepatchBounces {
        WriteBytes(0x004736E2, &depatch);
    } else {
        WriteBytes(0x004736E2, &original);
    }
}

pub fn sendBouncesToggleEasy() {
    let depatch = [0x74u8, 0x14];
    let depatch2 = [0xEBu8, 0x35];
    let original = [0x75u8, 0x14];
    let original2 = [0x75u8, 0x35];
    if vars().enableDepatchBouncesEasy {
        WriteBytes(0x004736E2, &depatch);
        WriteBytes(0x004736F6, &depatch2);
    } else {
        WriteBytes(0x004736E2, &original);
        WriteBytes(0x004736F6, &original2);
    }
}

/// `getPlayerName` — pointer into the game's client-name table (empty string if
/// out of range).
pub fn getPlayerName(client: i32) -> *const u8 {
    static EMPTY: [u8; 1] = [0];
    if client < 0 || client > 17 {
        return EMPTY.as_ptr();
    }
    (0x99786C + (client as usize * 0x52C)) as *const u8
}

pub fn loadPlayerNames() {
    let v = vars();
    for i in 0..=17usize {
        let src = getPlayerName(i as i32);
        // Read up to 39 chars (original copies into a 40-byte buffer). The
        // original guards this with SEH; here we just bound the scan. Only runs
        // while in-game, where the table is mapped.
        let mut name: Vec<u8> = Vec::new();
        unsafe {
            let mut k = 0usize;
            while k < 39 && *src.add(k) != 0 {
                name.push(*src.add(k));
                k += 1;
            }
        }
        let name_str = String::from_utf8_lossy(&name);
        let formatted = format!("Client({i}){name_str}");
        let dst = &mut v.playerName[i];
        *dst = [0; 64];
        let bytes = formatted.as_bytes();
        let n = bytes.len().min(63);
        dst[..n].copy_from_slice(&bytes[..n]);
    }
}

pub fn sendCustomPort() {
    SetDvarInt(dv::net_port, vars().customPort);
}

pub fn doIronSight() {
    let bytes1 = [0x00u8, 0x00];
    let bytes2 = [0x00u8];
    let bytes3 = [0x3Eu8, 0x16];
    let bytes4 = [0x40u8];

    unsafe {
        if vars().ironSightIntervention && isCheytacAcogWeapon() {
            SetDvarFloat(dv::cg_gun_z, 1.0);
            SetDvarFloat(dv::cg_gun_y, 0.0);
            SetDvarFloat(dv::cg_gun_x, -1.0);
            core::ptr::copy_nonoverlapping(bytes1.as_ptr(), 0x2516392D as *mut u8, 2);
            core::ptr::copy_nonoverlapping(bytes1.as_ptr(), 0x251639AD as *mut u8, 2);
            core::ptr::copy_nonoverlapping(bytes1.as_ptr(), 0x2516396D as *mut u8, 2);
            core::ptr::copy_nonoverlapping(bytes1.as_ptr(), 0x251639ED as *mut u8, 2);
            core::ptr::copy_nonoverlapping(bytes2.as_ptr(), 0x25162F1F as *mut u8, 1);
            strcpy_bounded(0x33CB8FEC, 28, b"Intervention IRON Sight");
        } else {
            SetDvarFloat(dv::cg_gun_z, 0.0);
            SetDvarFloat(dv::cg_gun_y, 0.0);
            SetDvarFloat(dv::cg_gun_x, 0.0);
            core::ptr::copy_nonoverlapping(bytes3.as_ptr(), 0x2516392D as *mut u8, 2);
            core::ptr::copy_nonoverlapping(bytes3.as_ptr(), 0x251639AD as *mut u8, 2);
            core::ptr::copy_nonoverlapping(bytes3.as_ptr(), 0x2516396D as *mut u8, 2);
            core::ptr::copy_nonoverlapping(bytes3.as_ptr(), 0x251639ED as *mut u8, 2);
            core::ptr::copy_nonoverlapping(bytes4.as_ptr(), 0x25162F1F as *mut u8, 1);
            strcpy_bounded(0x33CB8FEC, 28, b"Intervention ACOG Sight");
        }
    }
}

pub fn writeSensitivity(sens: f32) {
    cbuf_str(&format!("sensitivity {sens};"));
}

pub fn readSensitivity() -> f32 {
    ReadDvarFloat(dv::sensitivity)
}

pub fn menuUITweaks() {
    unsafe {
        strcpy_bounded(0x33BD7AF4, 28, b"^1d^7ismay's ^1degen^7 tool"); // ITnet
        strcpy_bounded(0x33BD80C0, 28, b"^2FIND GAME"); // Find game
        strcpy_bounded(0x33BD8519, 28, b"^1PRIVATE SESH"); // private match
        strcpy_bounded(0x33BD8A99, 28, b"^3CREATE A CLASS"); // create a class
        strcpy_bounded(0x33BD9BE0, 28, b"^4STREAKS/CALLSIGNS"); // killstreaks
        strcpy_bounded(0x33BDA69E, 28, b"^5BARRACKS OBAMA"); // barracks
        strcpy_bounded(0x33BDAFE1, 28, b"^6INVITE PEEPS"); // invite
        strcpy_bounded(0x33BE3F80, 28, b"^0PLAYLISTS"); // playlists
        strcpy_bounded(0x33C0759C, 28, b"^2START"); // start game
        strcpy_bounded(0x33C079A5, 28, b"^1SETTINGS"); // settings
        strcpy_bounded(0x33CBD9E6, 28, b"dismay >"); // title iron lungs
    }
}

pub fn sendPingText() {
    SetDvarInt(
        dv::cg_scoreboardPingText,
        if vars().pingText { 1 } else { 0 },
    );
}

pub fn sendProfileStats() {
    doSaveBarracks();
}

pub fn giveAmmo() {
    for i in 0..18usize {
        let step = i * 0x366C;
        unsafe {
            wr::<i32>(off::PrimaryMagAmmo + step, i32::MAX);
            wr::<i32>(off::PrimaryReserveAmmo + step, i32::MAX);
            wr::<i32>(off::SecondaryAmmoReserve + step, i32::MAX);
            wr::<i32>(off::SecondaryLeftGunMagAmmo + step, i32::MAX);
            wr::<i32>(off::SecondaryRightGunMagAmmo + step, i32::MAX);
            wr::<i32>(off::PrimNade + step, i32::MAX);
            wr::<i32>(off::StunNade + step, i32::MAX);
        }
    }
}

pub fn sendSprintScale() {
    SetDvarFloat(dv::player_sprintSpeedScale, vars().sprintScale);
}
pub fn sendKnockbackScale() {
    SetDvarFloat(dv::g_knockback, vars().knockbackScale);
}
pub fn sendBackSpeedScale() {
    SetDvarFloat(dv::player_backSpeedScale, vars().backSpeedScale);
}

pub fn sendViewModel() {
    let v = vars();
    SetDvarFloat(dv::cg_gun_x, v.fcg_gun_x);
    SetDvarFloat(dv::cg_gun_y, v.fcg_gun_y);
    SetDvarFloat(dv::cg_gun_z, v.fcg_gun_z);
}

pub fn forceTeamChange() {
    let id = unsafe { rd::<i32>(off::bullshit2) };
    cbuf_str(&format!("mr {id} 2 spectator\n"));
}

pub fn sendUnlockAllClients() {
    for i in 0..=17i32 {
        for cmd in crate::unlock_all::UNLOCK_ALL_CMDS {
            unsafe { funcs::SV_GameSendServerCommand(i, 0, cmd.as_ptr() as *mut u8) }
        }
    }
}

pub fn readGameString(address: usize, max_length: i32) -> String {
    let max_length = max_length.max(0) as usize;
    unsafe {
        let mut old: u32 = 0;
        win32::VirtualProtect(
            address as *mut c_void,
            max_length,
            win32::PAGE_EXECUTE_READWRITE,
            &mut old,
        );
        let mut result = String::new();
        for i in 0..max_length {
            let c = *((address + i) as *const u8);
            if c == 0 {
                break;
            }
            result.push(c as char);
        }
        win32::VirtualProtect(address as *mut c_void, max_length, old, &mut old);
        result
    }
}

pub fn getLocalClientNum() -> i32 {
    let self_name = readGameString(off::clientName, 256);
    if self_name.is_empty() {
        return 0;
    }
    for i in 0..MAX_SCOREBOARD_CLIENTS as i32 {
        let other = getPlayerName(i);
        if unsafe { cstr_eq_ignore_case(self_name.as_bytes(), other) } {
            return i;
        }
    }
    0
}

/// Case-insensitive compare of a Rust byte slice against a NUL-terminated C
/// string (`_stricmp` equality).
unsafe fn cstr_eq_ignore_case(a: &[u8], b: *const u8) -> bool {
    let mut i = 0usize;
    loop {
        let bc = *b.add(i);
        if i == a.len() {
            return bc == 0;
        }
        let ac = a[i];
        if ac.eq_ignore_ascii_case(&bc) {
            // matched (also covers both being 0)
        } else {
            return false;
        }
        if bc == 0 {
            return true;
        }
        i += 1;
    }
}

pub fn getScoreboardEntries() -> Vec<score_t> {
    unsafe {
        let count = rd::<i32>(off::cg_scoreboardPlayerCount);
        if count <= 0 {
            return Vec::new();
        }
        let limit = (count as usize).min(MAX_SCOREBOARD_CLIENTS);
        let entries = off::cg_scoreboardEntries as *const score_t;
        let mut out = Vec::with_capacity(limit);
        for i in 0..limit {
            out.push(*entries.add(i));
        }
        out
    }
}

pub fn isInGameNotSpectating() -> bool {
    unsafe {
        let flags = rd::<i32>(off::CG_OTHER_FLAGS);
        (flags & off::OTHER_FLAG_ISINGAME) != 0 && (flags & off::OTHER_FLAG_SPECTATING) == 0
    }
}

// --- feature worker thread --------------------------------------------------

static WORKER_STOP: AtomicBool = AtomicBool::new(true);
static WORKER: Mutex<Option<JoinHandle<()>>> = Mutex::new(None);

fn syncGameMouseCapture() {
    unsafe {
        if gui_state::open() {
            wr::<u8>(off::mouse_enable, 0);
        } else {
            wr::<u8>(off::mouse_enable, 1);
        }
    }
}

fn functionWorkerLoop() {
    let mut last_menu_tweak_ms: u64 = 0;
    while !WORKER_STOP.load(Ordering::Acquire) {
        if !gui_state::setup() {
            unsafe { win32::Sleep(10) };
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
        if vars().enableMouseOneToOne {
            sendFOVMin();
        }
        if vars().ironSightIntervention {
            doIronSight();
        }

        let now = unsafe { win32::GetTickCount64() };
        if now - last_menu_tweak_ms >= 250 {
            menuUITweaks();
            last_menu_tweak_ms = now;
        }

        unsafe { win32::Sleep(1) };
    }
}

pub fn startFeatureWorker() {
    stopFeatureWorker();
    WORKER_STOP.store(false, Ordering::Release);
    let handle = std::thread::spawn(functionWorkerLoop);
    *WORKER.lock().unwrap() = Some(handle);
}

pub fn stopFeatureWorker() {
    WORKER_STOP.store(true, Ordering::Release);
    if let Some(handle) = WORKER.lock().unwrap().take() {
        let _ = handle.join();
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn xp_table_segment_boundaries() {
        // Out-of-range
        assert_eq!(xpToAdvanceFromRank(0), 0);
        assert_eq!(xpToAdvanceFromRank(71), 0);
        // Segment 1 (1..=10): 500 + 700*(n-1)
        assert_eq!(xpToAdvanceFromRank(1), 500);
        assert_eq!(xpToAdvanceFromRank(10), 6800);
        // Segment 2 (11..=29): 7800 + 1000*(n-11)
        assert_eq!(xpToAdvanceFromRank(11), 7800);
        assert_eq!(xpToAdvanceFromRank(29), 25800);
        // Segment 3 (30..=49): 27000 + 1200*(n-30)
        assert_eq!(xpToAdvanceFromRank(30), 27000);
        assert_eq!(xpToAdvanceFromRank(49), 49800);
        // Segment 4 (50..=70): 51300 + 1500*(n-50)
        assert_eq!(xpToAdvanceFromRank(50), 51300);
        assert_eq!(xpToAdvanceFromRank(70), 81300);
    }

    #[test]
    fn total_xp_accumulates() {
        assert_eq!(totalXpAtRank(1), 0);
        assert_eq!(totalXpAtRank(0), 0);
        assert_eq!(totalXpAtRank(2), 500);
        assert_eq!(totalXpAtRank(3), 500 + 1200);
        // clamps above 70 to the rank-70 total
        assert_eq!(totalXpAtRank(99), totalXpAtRank(70));
    }
}
