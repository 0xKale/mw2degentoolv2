//! The non-rendering parts of the `gui::` namespace that other modules read —
//! chiefly the `open` / `setup` flags the feature worker polls. The actual
//! window/device/ImGui state lives in the rendering layer.

use core::sync::atomic::{AtomicBool, Ordering};

/// `gui::open` — menu visible. Starts true, toggled by the INSERT key.
static OPEN: AtomicBool = AtomicBool::new(true);
/// `gui::setup` — ImGui + hooks have been initialised.
static SETUP: AtomicBool = AtomicBool::new(false);

#[inline]
pub fn open() -> bool {
    OPEN.load(Ordering::Relaxed)
}
#[inline]
pub fn set_open(v: bool) {
    OPEN.store(v, Ordering::Relaxed);
}
#[inline]
pub fn toggle_open() {
    OPEN.fetch_xor(true, Ordering::Relaxed);
}

#[inline]
pub fn setup() -> bool {
    SETUP.load(Ordering::Acquire)
}
#[inline]
pub fn set_setup(v: bool) {
    SETUP.store(v, Ordering::Release);
}
