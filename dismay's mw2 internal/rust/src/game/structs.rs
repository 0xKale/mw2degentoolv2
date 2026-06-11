//! In-memory game structures. Port of `src/game/iw4structs.hpp`.
//!
//! Layouts must match the game exactly, so every struct is `#[repr(C)]` and the
//! original `static_assert` size checks are reproduced as compile-time const
//! assertions.

#![allow(non_camel_case_types, dead_code)]

use core::ffi::c_void;

/// `iw4::DLCDef` — a source row used to populate [`DLCList`].
#[derive(Clone, Copy)]
pub struct DLCDef {
    pub a2: i32,
    pub name: &'static [u8],
}

/// `iw4::DLCList` — the in-game DLC table entry (stride 136).
#[repr(C)]
#[derive(Clone, Copy)]
pub struct DLCList {
    pub name: [u8; 128],
    pub a2: i32,
    pub flag1: u8,
    pub flag2: u8,
    pub pad: [u8; 2],
}

const _: () = assert!(
    core::mem::size_of::<DLCList>() == 136,
    "DLCList stride is 136 bytes innit"
);

impl DLCList {
    pub const fn zeroed() -> Self {
        Self {
            name: [0; 128],
            a2: 0,
            flag1: 0,
            flag2: 0,
            pad: [0; 2],
        }
    }
}

/// `iw4::score_t` — one scoreboard row (44 bytes).
#[repr(C)]
#[derive(Clone, Copy, Default)]
pub struct score_t {
    pub clientNum: i32, // [0]
    pub score: i32,     // [1]
    pub ping: i32,      // [2]
    pub deaths: i32,    // [3]
    pub team: i32,      // [4]  TEAM_ALLIES=1, TEAM_AXIS=2, TEAM_SPECTATOR=3
    pub kills: i32,     // [5]
    pub rank: i32,      // [6]  prestige/rank id from client state
    pub assists: i32,   // [7]
    pub skill: i32,     // [8]
    pub rankIcon: i32,  // [9]  Material* as int level
    pub rankIcon2: i32, // [10] Material* as int prestige
}

const _: () = assert!(core::mem::size_of::<score_t>() == 44);

pub const MAX_SCOREBOARD_CLIENTS: usize = 18;
pub const CG_CLIENT_STRIDE: i32 = 331;

/// `iw4::weapon_complete_def_t`
#[repr(C)]
pub struct weapon_complete_def_t {
    pub name: *const u8,
    pub weaponDef: *mut c_void,
    pub localizedName: *const u8,
}
