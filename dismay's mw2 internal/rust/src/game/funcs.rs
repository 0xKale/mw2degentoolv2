//! Engine function pointers. Port of `src/game/iw4hooks.{h,cpp}` and
//! `src/game/functions.{hpp,cpp}`.
//!
//! In the C++ these are global `typedef`'d pointers initialised to absolute
//! addresses, e.g. `SV_GameSendServerCommand = (…)0x588340`. Here each is a thin
//! `unsafe` wrapper that transmutes the address to the correctly-typed `cdecl`
//! function and calls it. The game ABI is `__cdecl`, which Rust spells
//! `extern "cdecl"`.

#![allow(non_snake_case, dead_code)]

use super::offsets;
use super::structs::weapon_complete_def_t;
use core::ffi::c_void;

// --- raw engine addresses (iw4hooks.cpp) -----------------------------------
const ADDR_SV_GAME_SEND_SERVER_COMMAND: usize = 0x588340;
const ADDR_CBUF_ADD_TEXT: usize = 0x563BE0;
const ADDR_CBUF_ADD_CALL: usize = 0x563C90;
const ADDR_OPEN_MENU: usize = 0x59DDE0;
const ADDR_SV_SPAWN_SERVER: usize = 0x589D90;
const ADDR_BALANCE_TEAMS: usize = 0x4D73B0;
const ADDR_MAP_RESTART: usize = 0x5850A0;
const ADDR_CL_ADD_RELIABLE_COMMAND: usize = 0x4C2DD0;

/// `void __cdecl SV_GameSendServerCommand(int clientNum, int reliable, char* command)`
#[inline]
pub unsafe fn SV_GameSendServerCommand(client_num: i32, reliable: i32, command: *mut u8) {
    let f: unsafe extern "cdecl" fn(i32, i32, *mut u8) =
        core::mem::transmute(ADDR_SV_GAME_SEND_SERVER_COMMAND);
    f(client_num, reliable, command)
}

/// `void __cdecl Cbuf_AddText(int localClientNum, const char* text)`
#[inline]
pub unsafe fn Cbuf_AddText(local_client_num: i32, text: *const u8) {
    let f: unsafe extern "cdecl" fn(i32, *const u8) = core::mem::transmute(ADDR_CBUF_ADD_TEXT);
    f(local_client_num, text)
}

/// `void __cdecl Cbuf_AddCall(int a1, void* a2)`
#[inline]
pub unsafe fn Cbuf_AddCall(a1: i32, a2: *mut c_void) {
    let f: unsafe extern "cdecl" fn(i32, *mut c_void) = core::mem::transmute(ADDR_CBUF_ADD_CALL);
    f(a1, a2)
}

/// `void __cdecl OpenMenu(int localClientNum, const char* menuName)`
#[inline]
pub unsafe fn OpenMenu(local_client_num: i32, menu_name: *const u8) {
    let f: unsafe extern "cdecl" fn(i32, *const u8) = core::mem::transmute(ADDR_OPEN_MENU);
    f(local_client_num, menu_name)
}

/// `void __cdecl SV_SpawnServer(char* name, int, int)`
#[inline]
pub unsafe fn SV_SpawnServer(name: *mut u8, a: i32, b: i32) {
    let f: unsafe extern "cdecl" fn(*mut u8, i32, i32) = core::mem::transmute(ADDR_SV_SPAWN_SERVER);
    f(name, a, b)
}

/// `void __cdecl BalanceTeams(void* PartyData_s_party)`
#[inline]
pub unsafe fn BalanceTeams(party: *mut c_void) {
    let f: unsafe extern "cdecl" fn(*mut c_void) = core::mem::transmute(ADDR_BALANCE_TEAMS);
    f(party)
}

/// `void __cdecl MapRestart(int fastRestart, int unknown)`
#[inline]
pub unsafe fn MapRestart(fast_restart: i32, unknown: i32) {
    let f: unsafe extern "cdecl" fn(i32, i32) = core::mem::transmute(ADDR_MAP_RESTART);
    f(fast_restart, unknown)
}

/// `void __cdecl CL_AddReliableCommand(int localClientNum, const char* command)`
#[inline]
pub unsafe fn CL_AddReliableCommand(local_client_num: i32, command: *const u8) {
    let f: unsafe extern "cdecl" fn(i32, *const u8) =
        core::mem::transmute(ADDR_CL_ADD_RELIABLE_COMMAND);
    f(local_client_num, command)
}

// --- weapon name helpers (game/functions.cpp) ------------------------------

#[inline]
unsafe fn cg() -> *mut c_void {
    offsets::CG as *mut c_void
}

/// `iw4::getViewmodelWeaponIndex()`
#[inline]
pub unsafe fn get_viewmodel_weapon_index() -> u32 {
    let f: unsafe extern "cdecl" fn(*mut c_void) -> u32 =
        core::mem::transmute(offsets::BG_GetViewmodelWeaponIndex);
    f(cg())
}

/// `iw4::getWeaponNameRaw(weaponIndex)` — null if the name is empty/missing.
#[inline]
pub unsafe fn get_weapon_name_raw(weapon_index: u32) -> *const u8 {
    let f: unsafe extern "cdecl" fn(u32) -> *mut weapon_complete_def_t =
        core::mem::transmute(offsets::BG_GetWeaponCompleteDef);
    let def = f(weapon_index);
    if def.is_null() || (*def).name.is_null() || *(*def).name == 0 {
        return core::ptr::null();
    }
    (*def).name
}

/// `iw4::getCurrentWeaponName()` — null if unavailable.
#[inline]
pub unsafe fn get_current_weapon_name() -> *const u8 {
    get_weapon_name_raw(get_viewmodel_weapon_index())
}
