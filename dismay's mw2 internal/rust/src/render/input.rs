//! Win32 input capture for the menu. Replaces `imgui_impl_win32`'s wnd-proc
//! handling: the subclassed window proc feeds messages here, and the renderer
//! takes a per-frame snapshot.

use crate::state::Racy;

const WM_MOUSEMOVE: u32 = 0x0200;
const WM_LBUTTONDOWN: u32 = 0x0201;
const WM_LBUTTONUP: u32 = 0x0202;
const WM_RBUTTONDOWN: u32 = 0x0204;
const WM_RBUTTONUP: u32 = 0x0205;
const WM_MBUTTONDOWN: u32 = 0x0207;
const WM_MBUTTONUP: u32 = 0x0208;
const WM_MOUSEWHEEL: u32 = 0x020A;

#[derive(Default)]
struct RawInput {
    mouse_pos: [f32; 2],
    down: [bool; 3],
    prev: [bool; 3],
    wheel: f32,
}

static INPUT: Racy<RawInput> = Racy::new(RawInput {
    mouse_pos: [0.0, 0.0],
    down: [false; 3],
    prev: [false; 3],
    wheel: 0.0,
});

/// Immutable per-frame view the widgets read.
#[derive(Clone, Copy, Default)]
pub struct InputSnapshot {
    pub mouse_pos: [f32; 2],
    pub down: [bool; 3],
    pub clicked: [bool; 3],
    pub released: [bool; 3],
    pub wheel: f32,
}

impl InputSnapshot {
    pub fn hovers(&self, min: [f32; 2], max: [f32; 2]) -> bool {
        self.mouse_pos[0] >= min[0]
            && self.mouse_pos[0] < max[0]
            && self.mouse_pos[1] >= min[1]
            && self.mouse_pos[1] < max[1]
    }
}

/// Feed one window message. Returns true if it was a mouse/wheel message the
/// menu consumed (so the caller can swallow it from the game while open).
pub fn feed(msg: u32, wparam: usize, lparam: isize) -> bool {
    let s = INPUT.get();
    match msg {
        WM_MOUSEMOVE => {
            let x = (lparam & 0xFFFF) as i16 as f32;
            let y = ((lparam >> 16) & 0xFFFF) as i16 as f32;
            s.mouse_pos = [x, y];
            true
        }
        WM_LBUTTONDOWN => {
            s.down[0] = true;
            true
        }
        WM_LBUTTONUP => {
            s.down[0] = false;
            true
        }
        WM_RBUTTONDOWN => {
            s.down[1] = true;
            true
        }
        WM_RBUTTONUP => {
            s.down[1] = false;
            true
        }
        WM_MBUTTONDOWN => {
            s.down[2] = true;
            true
        }
        WM_MBUTTONUP => {
            s.down[2] = false;
            true
        }
        WM_MOUSEWHEEL => {
            let delta = ((wparam >> 16) & 0xFFFF) as i16 as f32 / 120.0;
            s.wheel += delta;
            true
        }
        _ => false,
    }
}

/// Compute edge events and produce the snapshot for this frame.
pub fn begin_frame() -> InputSnapshot {
    let s = INPUT.get();
    let mut snap = InputSnapshot {
        mouse_pos: s.mouse_pos,
        down: s.down,
        clicked: [false; 3],
        released: [false; 3],
        wheel: s.wheel,
    };
    for i in 0..3 {
        snap.clicked[i] = s.down[i] && !s.prev[i];
        snap.released[i] = !s.down[i] && s.prev[i];
    }
    s.prev = s.down;
    s.wheel = 0.0;
    snap
}
