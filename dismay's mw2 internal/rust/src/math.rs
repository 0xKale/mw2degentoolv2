//! Minimal math types. `Vec4` is laid out exactly like ImGui's `ImVec4`
//! (`struct { float x, y, z, w; }`) so the same value can be handed to the
//! rendering layer's FFI without conversion.

/// ABI-compatible with `ImVec4`.
#[repr(C)]
#[derive(Clone, Copy, Debug, PartialEq, Default)]
pub struct Vec4 {
    pub x: f32,
    pub y: f32,
    pub z: f32,
    pub w: f32,
}

impl Vec4 {
    pub const fn new(x: f32, y: f32, z: f32, w: f32) -> Self {
        Self { x, y, z, w }
    }

    /// Mirrors `ImColor(r, g, b)` — 8-bit components, alpha forced to 255.
    pub const fn rgb(r: u8, g: u8, b: u8) -> Self {
        Self::rgba(r, g, b, 255)
    }

    /// Mirrors `ImColor(r, g, b, a)`.
    pub const fn rgba(r: u8, g: u8, b: u8, a: u8) -> Self {
        Self {
            x: r as f32 / 255.0,
            y: g as f32 / 255.0,
            z: b as f32 / 255.0,
            w: a as f32 / 255.0,
        }
    }
}
