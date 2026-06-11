//! Rust port of *dismay's mw2 internal* — a 32-bit DLL injected into Modern
//! Warfare 2 (IW4 engine).
//!
//! Module layout mirrors the C++ `src/` tree:
//!   * [`game`]                — `src/game/*` + `src/dismay/offsets.hpp`
//!   * [`state`]               — the `vars` globals + `colors::accent_color`
//!   * [`functions`]           — `src/dismay/functions.cpp` (game logic)
//!   * [`config`]              — `src/dismay/config.cpp`
//!   * [`crosshair_sharecode`] — `src/dismay/crosshair_sharecode.cpp`
//!   * [`gui_state`]           — the non-rendering parts of `gui::` the worker reads
//!
//! The ImGui/DirectX9 rendering + MinHook layer (`gui.cpp`, `framework/`,
//! `menu/`, `hooks.hpp`) is the remaining phase; see `STATUS.md`.

#![allow(non_snake_case)]

use core::ffi::c_void;

pub mod assets;
pub mod config;
pub mod crosshair_sharecode;
pub mod functions;
pub mod game;
pub mod gui;
pub mod gui_state;
pub mod hooks;
pub mod inline_hook;
pub mod log;
pub mod math;
pub mod render;
pub mod state;
mod unlock_all;
pub mod win32;

use win32::DLL_PROCESS_ATTACH;

/// Bootstrap thread body. Mirrors `Setup()` in `dllmain.cpp`, minus the GUI
/// bring-up which belongs to the rendering phase. Loads config and starts the
/// feature worker once the menu device is up.
unsafe extern "system" fn bootstrap(_param: *mut c_void) -> u32 {
    log::init_panic_hook();
    log::log("bootstrap: start");
    state::init_default_player_names();

    // Acquire the shared D3D9 device vtable, then VMT-hook EndScene/Reset. From
    // there the EndScene hook drives setup, the worker, config load, and the
    // menu — exactly as the C++ `Setup()` does.
    if gui::acquire_vtable() {
        log::log("bootstrap: acquire_vtable OK");
        if hooks::install() {
            log::log("bootstrap: hooks installed");
        } else {
            log::log("bootstrap: hooks install FAILED");
        }
    } else {
        log::log("bootstrap: acquire_vtable FAILED");
    }
    0
}

#[no_mangle]
pub extern "system" fn DllMain(instance: *mut c_void, reason: u32, _reserved: *mut c_void) -> i32 {
    if reason == DLL_PROCESS_ATTACH {
        unsafe {
            win32::DisableThreadLibraryCalls(instance);
            let thread = win32::CreateThread(
                core::ptr::null_mut(),
                0,
                Some(bootstrap),
                instance,
                0,
                core::ptr::null_mut(),
            );
            if !thread.is_null() {
                win32::CloseHandle(thread);
            }
        }
    }
    1 // TRUE
}
