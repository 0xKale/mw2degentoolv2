//! Pure-Rust renderer: draw list + DX9 backend + font atlas + input, plus the
//! menu drawing. Replaces ImGui, its DX9/Win32 backends, and the `framework/`
//! widget layer.

pub mod context;
pub mod draw_list;
pub mod dx9;
pub mod font;
pub mod gif;
pub mod input;
pub mod menu;
pub mod widgets;

use crate::gui_state;
use crate::state::Racy;
use context::Ui;
use core::ffi::c_void;
use core::sync::atomic::{AtomicU64, Ordering};
use draw_list::DrawList;
use dx9::Renderer;
use font::Fonts;
use windows::core::Interface;
use windows::Win32::Graphics::Direct3D9::{IDirect3DDevice9, D3DVIEWPORT9};

static FONTS: Racy<Option<Fonts>> = Racy::new(None);
static RENDERER: Racy<Renderer> = Racy::new(Renderer::new());
static LAST_TIME_MS: AtomicU64 = AtomicU64::new(0);

/// Per-frame menu UI state (the `misc` namespace in the C++) plus immediate-mode
/// interaction state (active widget, scroll).
pub struct MenuState {
    pub tab_count: i32,
    pub active_tab_count: i32,
    pub anim_tab: f32,
    /// Currently-dragged widget id (sliders), 0 = none.
    pub active_id: u64,
    /// Vertical scroll of the body, per tab.
    pub scroll_y: [f32; 5],
}
pub static MENU: Racy<MenuState> = Racy::new(MenuState {
    tab_count: 0,
    active_tab_count: 0,
    anim_tab: 0.0,
    active_id: 0,
    scroll_y: [0.0; 5],
});

fn fonts() -> &'static Fonts {
    let slot = FONTS.get();
    if slot.is_none() {
        let f = Fonts::build();
        crate::log::log(&format!(
            "fonts: built atlas {}x{} (body/bold/title/icons baked)",
            f.width, f.height
        ));
        *slot = Some(f);
    }
    slot.as_ref().unwrap()
}

fn viewport_size(device_ptr: *mut c_void) -> Option<[f32; 2]> {
    let raw = device_ptr;
    let device = unsafe { IDirect3DDevice9::from_raw_borrowed(&raw) }?;
    let mut vp = D3DVIEWPORT9::default();
    if unsafe { device.GetViewport(&mut vp) }.is_err() {
        return None;
    }
    if vp.Width == 0 || vp.Height == 0 {
        return None;
    }
    Some([vp.Width as f32, vp.Height as f32])
}

/// Build the UI for this frame and render it. Called from the `EndScene` hook.
pub fn render_frame(device_ptr: *mut c_void) {
    let display = match viewport_size(device_ptr) {
        Some(d) => d,
        None => return,
    };

    {
        use core::sync::atomic::AtomicBool;
        static LOGGED: AtomicBool = AtomicBool::new(false);
        if !LOGGED.swap(true, Ordering::Relaxed) {
            crate::log::log(&format!(
                "render_frame: first frame, display={}x{}, building fonts…",
                display[0], display[1]
            ));
        }
    }

    let now = unsafe { crate::win32::GetTickCount64() };
    let last = LAST_TIME_MS.swap(now, Ordering::Relaxed);
    let dt = if last == 0 || now <= last {
        1.0 / 60.0
    } else {
        ((now - last) as f32 / 1000.0).min(0.25)
    };

    let fonts = fonts();
    let input = input::begin_frame();

    let mut ui = Ui::new(DrawList::new(), fonts, input, display, dt);
    ui.dl.begin_frame(display, fonts.white_uv);

    // Crosshair overlay always draws (DrawCrosshairOverlay); the menu only when open.
    menu::draw_crosshair_overlay(&mut ui);
    if gui_state::open() {
        menu::draw(&mut ui);
    }

    let r = RENDERER.get();
    if gui_state::open() {
        if let Some((rgba, w, h)) = gif::current_frame() {
            r.ensure_lain(device_ptr, rgba, w, h);
        }
    }
    r.render(device_ptr, &ui.dl, fonts, display);
}

/// `Reset` hook: drop device-pool resources before the device is reset.
pub fn invalidate() {
    RENDERER.get().invalidate();
}
