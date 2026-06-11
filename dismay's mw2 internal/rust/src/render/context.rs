//! Per-frame immediate-mode context handed to the menu/widget code: the draw
//! list, the font set, an input snapshot, and small layout/interaction helpers
//! (the bits of `ImGui`/`ImGui::GetWindowDrawList` the widgets actually use).

use super::draw_list::{col_vec4, Color, DrawList};
use super::font::Fonts;
use super::input::InputSnapshot;
use crate::math::Vec4;

pub struct Ui<'a> {
    pub dl: DrawList,
    pub fonts: &'a Fonts,
    pub input: InputSnapshot,
    pub display: [f32; 2],
    pub dt: f32,
    /// Global alpha multiplier (style `Alpha`), for fade animations.
    pub alpha: f32,
    /// Layout cursor (screen-space top-left of the next widget).
    pub cursor: [f32; 2],
    /// Current panel inner width (the widgets' "window width").
    pub window_w: f32,
}

impl<'a> Ui<'a> {
    pub fn new(dl: DrawList, fonts: &'a Fonts, input: InputSnapshot, display: [f32; 2], dt: f32) -> Self {
        Ui {
            dl,
            fonts,
            input,
            display,
            dt,
            alpha: 1.0,
            cursor: [0.0, 0.0],
            window_w: 299.0,
        }
    }

    /// Pack a normalized color with the current global alpha.
    #[inline]
    pub fn col(&self, v: Vec4) -> Color {
        col_vec4(v, self.alpha)
    }

    /// `(hovered, pressed)` for a rectangle, using the mouse snapshot.
    pub fn button_behavior(&self, min: [f32; 2], max: [f32; 2]) -> (bool, bool) {
        let hovered = self.input.hovers(min, max);
        (hovered, hovered && self.input.clicked[0])
    }
}

/// `ImLerp(a, b, t)`.
#[inline]
pub fn lerp(a: f32, b: f32, t: f32) -> f32 {
    a + (b - a) * t
}
