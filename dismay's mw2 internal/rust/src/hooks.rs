//! D3D9 `EndScene` / `Reset` interception. Port of `hooks.hpp`.
//!
//! The C++ uses MinHook **inline** hooks, and so do we (via [`crate::inline_hook`]).
//! VMT-hooking a probe device's vtable does *not* intercept the game's device —
//! confirmed on-target — because that vtable is a separate instance. Inline
//! hooks patch the shared `d3d9.dll` code every device calls.

use crate::{functions, gui, gui_state};
use core::ffi::c_void;
use core::sync::atomic::{AtomicBool, AtomicUsize, Ordering};

static ORIG_ENDSCENE: AtomicUsize = AtomicUsize::new(0);
static ORIG_RESET: AtomicUsize = AtomicUsize::new(0);
static BG_STARTED: AtomicBool = AtomicBool::new(false);

type EndSceneFn = unsafe extern "system" fn(*mut c_void) -> i32;
type ResetFn = unsafe extern "system" fn(*mut c_void, *mut c_void) -> i32;

/// `hooks::Setup`. Requires [`gui::acquire_vtable`] to have resolved the
/// EndScene/Reset addresses.
pub fn install() -> bool {
    let es = gui::endscene_addr();
    let rs = gui::reset_addr();
    if es == 0 {
        crate::log::log("hooks::install: no EndScene address");
        return false;
    }

    match unsafe { crate::inline_hook::install(es, end_scene_hook as *const () as usize) } {
        Some(orig) => ORIG_ENDSCENE.store(orig, Ordering::Release),
        None => {
            crate::log::log("hooks::install: EndScene hook FAILED");
            return false;
        }
    }

    if rs != 0 {
        match unsafe { crate::inline_hook::install(rs, reset_hook as *const () as usize) } {
            Some(orig) => ORIG_RESET.store(orig, Ordering::Release),
            None => crate::log::log("hooks::install: Reset hook failed (continuing without it)"),
        }
    }

    crate::log::log("hooks::install: inline hooks installed");
    true
}

/// `hooks::TryStartBackgroundServices` — start the worker once setup completes.
fn try_start_background_services() {
    if BG_STARTED.load(Ordering::Acquire) {
        return;
    }
    if !gui_state::setup() {
        return;
    }
    if BG_STARTED.swap(true, Ordering::AcqRel) {
        return;
    }
    functions::startFeatureWorker();
    // dedigamer::init() — pending (network/account feature).
}

/// `hooks::EndScene`. Mirrors the original control flow: call the original,
/// lazily set up the menu, start background services on the first setup frame,
/// then render.
unsafe extern "system" fn end_scene_hook(device: *mut c_void) -> i32 {
    static FIRED: AtomicBool = AtomicBool::new(false);
    if !FIRED.swap(true, Ordering::Relaxed) {
        crate::log::log("EndScene: first call (hook is live)");
    }

    let orig: EndSceneFn = core::mem::transmute(ORIG_ENDSCENE.load(Ordering::Acquire));
    let result = orig(device);

    let was_setup = gui_state::setup();
    if !was_setup {
        gui::setup_menu(device);
    }
    if !gui_state::setup() {
        return result;
    }
    if !was_setup {
        try_start_background_services();
        return result;
    }

    gui::render(device);
    result
}

/// `hooks::Reset`. The renderer's device-object invalidate/recreate is added
/// with the renderer; for now we just forward.
unsafe extern "system" fn reset_hook(device: *mut c_void, params: *mut c_void) -> i32 {
    if ORIG_RESET.load(Ordering::Acquire) == 0 || device.is_null() || params.is_null() {
        return -2005530516; // D3DERR_INVALIDCALL
    }
    // Drop D3DPOOL_DEFAULT resources before the device is reset, recreate lazily.
    crate::render::invalidate();
    let orig: ResetFn = core::mem::transmute(ORIG_RESET.load(Ordering::Acquire));
    orig(device, params)
}
