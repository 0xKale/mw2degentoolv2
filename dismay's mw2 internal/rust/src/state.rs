//! Global mutable settings — the `vars` namespace from the page headers, plus
//! `colors::accent_color` (which `config` persists).
//!
//! The original is a pile of plain C++ globals mutated from the UI thread and
//! read from the feature-worker thread; it relies on those word-sized
//! reads/writes being benignly racy. We reproduce that exactly with a single
//! interior-mutable global rather than pretending the access is synchronised.
//! Field names are kept identical to the C++ for a 1:1 mapping.

#![allow(non_snake_case, non_upper_case_globals, dead_code)]

use crate::math::Vec4;
use core::cell::UnsafeCell;

/// Interior-mutable, `Sync` container for a single global, mirroring a plain
/// C++ global. Callers keep the same discipline the C++ relies on: short,
/// non-overlapping field reads/writes.
pub struct Racy<T>(UnsafeCell<T>);
unsafe impl<T> Sync for Racy<T> {}
impl<T> Racy<T> {
    pub const fn new(value: T) -> Self {
        Racy(UnsafeCell::new(value))
    }
    #[allow(clippy::mut_from_ref)]
    #[inline]
    pub fn get(&self) -> &mut T {
        unsafe { &mut *self.0.get() }
    }
}

/// `vars::*`
#[repr(C)]
pub struct Vars {
    // -- page_main --
    pub enableTextChat: bool,
    pub enableMouseOneToOne: bool,
    pub ironSightIntervention: bool,

    pub noSun: bool,
    pub drawCamo: bool,
    pub drawFog: bool,
    pub drawBullets: bool,
    pub movieMode: bool,
    pub clearGlass: bool,
    pub pingText: bool,

    pub mouseSensitivity: f32,
    pub defaultFovMin: f32,

    pub framesPerSecond: i32,
    pub fieldOfView: f32,
    pub mapSize: f32,

    pub selectedFullbrightMode: i32,

    pub console: [u8; 256],

    pub enableDLC: bool,
    pub enableCustomPort: bool,
    pub customPort: i32,

    pub fullbright: i32,
    pub lightmap: i32,

    pub enableCrosshair: bool,
    pub crosshair_color: Vec4,
    pub crosshairOutline: bool,
    pub crosshairGap: f32,
    pub crosshairLength: f32,
    pub crosshairThickness: f32,
    pub crosshairOutlineThickness: f32,
    pub crosshairScale: f32,
    pub crosshairLengthScale: f32,
    pub crosshairGapScale: f32,
    pub crosshairCenterDot: bool,
    pub crosshairTStyle: bool,
    pub csShareCodeInput: [u8; 48],

    pub fcg_gun_x: f32,
    pub fcg_gun_y: f32,
    pub fcg_gun_z: f32,

    // -- page_host --
    pub playerName: [[u8; 64]; 18],
    pub serverCommand: [u8; 256],
    pub selectedMap: i32,
    pub selectedGamemode: i32,
    pub maxPlayers: i32,
    pub FFATeamFix: bool,
    pub enableHostHotkeys: bool,
    pub sprintScale: f32,
    pub knockbackScale: f32,
    pub backSpeedScale: f32,
    pub enableDepatchBounces: bool,
    pub enableDepatchBouncesEasy: bool,
    pub enableDepatchElevators: bool,
    pub proneCap360: bool,
    pub ladderCap360: bool,

    // -- page_account --
    pub prestige: i32,
    pub rank: i32,
    pub wins: i32,
    pub losses: i32,
    pub ties: i32,
    pub winStreak: i32,
    pub kills: i32,
    pub headshots: i32,
    pub assists: i32,
    pub killStreak: i32,
    pub deaths: i32,
    pub timePlayed: i32,

    // -- colors::accent_color (persisted by config) --
    pub accent_color: Vec4,
}

impl Vars {
    pub const fn new() -> Self {
        Self {
            enableTextChat: true,
            enableMouseOneToOne: false,
            ironSightIntervention: false,

            noSun: true,
            drawCamo: true,
            drawFog: true,
            drawBullets: true,
            movieMode: false,
            clearGlass: false,
            pingText: true,

            mouseSensitivity: 0.0, // set from readSensitivity() at startup
            defaultFovMin: 1.0,

            framesPerSecond: 400,
            fieldOfView: 90.0,
            mapSize: 1.0,

            selectedFullbrightMode: 1,

            console: [0; 256],

            enableDLC: false,
            enableCustomPort: false,
            customPort: 28961,

            fullbright: 0,
            lightmap: 1,

            enableCrosshair: false,
            crosshair_color: Vec4::rgb(225, 255, 255),
            crosshairOutline: true,
            crosshairGap: 1.8,
            crosshairLength: 3.5,
            crosshairThickness: 1.3,
            crosshairOutlineThickness: 1.0,
            crosshairScale: 1.0,
            crosshairLengthScale: 1.0,
            crosshairGapScale: 1.0,
            crosshairCenterDot: false,
            crosshairTStyle: false,
            csShareCodeInput: [0; 48],

            fcg_gun_x: 0.0,
            fcg_gun_y: 0.0,
            fcg_gun_z: 0.0,

            playerName: [[0; 64]; 18],
            serverCommand: [0; 256],
            selectedMap: 0,
            selectedGamemode: 0,
            maxPlayers: 18,
            FFATeamFix: false,
            enableHostHotkeys: false,
            sprintScale: 1.0,
            knockbackScale: 1000.0,
            backSpeedScale: 0.69,
            enableDepatchBounces: false,
            enableDepatchBouncesEasy: false,
            enableDepatchElevators: false,
            proneCap360: false,
            ladderCap360: false,

            prestige: 10,
            rank: 70,
            wins: 0,
            losses: 0,
            ties: 0,
            winStreak: 0,
            kills: 0,
            headshots: 0,
            assists: 0,
            killStreak: 0,
            deaths: 0,
            timePlayed: 0,

            accent_color: Vec4::rgb(255, 255, 255),
        }
    }
}

static VARS: Racy<Vars> = Racy::new(Vars::new());

/// Access the global settings, mirroring direct `vars::field` access in C++.
#[inline]
pub fn vars() -> &'static mut Vars {
    VARS.get()
}

// --- immutable lookup tables (const data, never mutated) --------------------

/// `vars::fullbrightModes` (UI labels).
pub const FULLBRIGHT_MODES: [&str; 5] = ["Invert", "Normal", "Super", "Slight", "Dullish"];

/// `vars::mapList` — engine map names, NUL-terminated for passing to
/// `SV_SpawnServer` as `char*`.
pub const MAP_LIST: [&[u8]; 26] = [
    b"mp_afghan\0",
    b"mp_derail\0",
    b"mp_estate\0",
    b"mp_favela\0",
    b"mp_highrise\0",
    b"mp_invasion\0",
    b"mp_checkpoint\0",
    b"mp_quarry\0",
    b"mp_rundown\0",
    b"mp_rust\0",
    b"mp_boneyard\0",
    b"mp_nightshift\0",
    b"mp_subbase\0",
    b"mp_terminal\0",
    b"mp_underpass\0",
    b"mp_brecourt\0",
    b"mp_complex\0",
    b"mp_crash\0",
    b"mp_compact\0",
    b"mp_overgrown\0",
    b"mp_storm\0",
    b"mp_abandon\0",
    b"mp_fuel2\0",
    b"mp_strike\0",
    b"mp_trailerpark\0",
    b"mp_vacant\0",
];

/// `vars::mapListDisplay` (UI labels).
pub const MAP_LIST_DISPLAY: [&str; 26] = [
    "Afghan",
    "Derail",
    "Estate",
    "Favela",
    "Highrise",
    "Invasion",
    "Karachi",
    "Quarry",
    "Rundown",
    "Rust",
    "Scrapyard",
    "Skidrow",
    "Sub Base",
    "Terminal",
    "Underpass",
    "Wasteland",
    "Bailout",
    "Crash",
    "Salvage",
    "Overgrown",
    "Storm",
    "Carnival",
    "Fuel",
    "Strike",
    "Trailer Park",
    "Vacant",
];

/// `vars::gamemodeList` (engine gametype tokens).
pub const GAMEMODE_LIST: [&str; 12] = [
    "dom", "war", "sd", "ffa", "koth", "dem", "sab", "ctf", "gtnw", "oneflag", "vip", "arena",
];

/// `vars::gamemodeListDisplay` (UI labels).
pub const GAMEMODE_LIST_DISPLAY: [&str; 12] = [
    "Domination",
    "Team Deathmatch",
    "Search and Destroy",
    "Free-For-All",
    "Headquarters",
    "Demolition",
    "Sabotage",
    "Capture the Flag",
    "Global Thermonuclear War",
    "One Flag CTF",
    "VIP",
    "Arena",
];

/// Placeholder client names (`vars::playerName` initialiser), overwritten by
/// `functions::load_player_names` once in-game.
pub const DEFAULT_PLAYER_NAMES: [&[u8]; 18] = [
    b"client0", b"client1", b"client2", b"client3", b"client4", b"client5", b"client6", b"client7",
    b"client8", b"client9", b"client10", b"client11", b"client12", b"client13", b"client14",
    b"client15", b"client16", b"client17",
];

/// Fill `vars::playerName` with the default placeholders (called once at init).
pub fn init_default_player_names() {
    let v = vars();
    for (i, name) in DEFAULT_PLAYER_NAMES.iter().enumerate() {
        let dst = &mut v.playerName[i];
        *dst = [0; 64];
        let n = name.len().min(63);
        dst[..n].copy_from_slice(&name[..n]);
    }
}
