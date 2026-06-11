//! Immediate-mode draw list — the pure-Rust equivalent of ImGui's `ImDrawList`.
//! Accumulates vertices/indices + draw commands (clip rect + texture) that the
//! DX9 backend renders. Geometry math mirrors ImGui (path → convex-poly fill,
//! polyline borders, per-corner rounding).

#![allow(dead_code)]

use crate::math::Vec4;

/// Packed RGBA, byte order R,G,B,A (same as ImGui's `IM_COL32`).
pub type Color = u32;

#[inline]
pub const fn col32(r: u8, g: u8, b: u8, a: u8) -> Color {
    (r as u32) | ((g as u32) << 8) | ((b as u32) << 16) | ((a as u32) << 24)
}

#[inline]
pub fn col_alpha(c: Color, alpha: f32) -> Color {
    let a = ((c >> 24) & 0xFF) as f32 * alpha.clamp(0.0, 1.0);
    (c & 0x00FF_FFFF) | ((a as u32) << 24)
}

/// `ImGui::GetColorU32(ImVec4)` — normalized float color → packed, premultiplied
/// by a global alpha (for the menu's fade animations).
#[inline]
pub fn col_vec4(v: Vec4, global_alpha: f32) -> Color {
    let f = |x: f32| (x.clamp(0.0, 1.0) * 255.0 + 0.5) as u8;
    col32(f(v.x), f(v.y), f(v.z), f(v.w * global_alpha))
}

#[inline]
pub fn v2(x: f32, y: f32) -> [f32; 2] {
    [x, y]
}

#[repr(C)]
#[derive(Clone, Copy)]
pub struct Vertex {
    pub pos: [f32; 2],
    pub col: Color,
    pub uv: [f32; 2],
}

#[derive(Clone, Copy)]
pub struct DrawCmd {
    pub clip: [f32; 4], // x0,y0,x1,y1
    pub texture: u8,    // 0 = white/font atlas, 1 = lain gif texture
    pub idx_offset: u32,
    pub elem_count: u32,
}

/// Corner-rounding flags (subset of `ImDrawFlags`).
pub mod corner {
    pub const TOP_LEFT: u32 = 1 << 0;
    pub const TOP_RIGHT: u32 = 1 << 1;
    pub const BOT_LEFT: u32 = 1 << 2;
    pub const BOT_RIGHT: u32 = 1 << 3;
    pub const TOP: u32 = TOP_LEFT | TOP_RIGHT;
    pub const BOT: u32 = BOT_LEFT | BOT_RIGHT;
    pub const ALL: u32 = TOP | BOT;
}

pub const TEX_FONT: u8 = 0;
pub const TEX_LAIN: u8 = 1;

pub struct DrawList {
    pub vtx: Vec<Vertex>,
    pub idx: Vec<u32>,
    pub cmds: Vec<DrawCmd>,
    clip_stack: Vec<[f32; 4]>,
    cur_tex: u8,
    /// UV of a white texel in the atlas (for untextured geometry).
    pub white_uv: [f32; 2],
    display_size: [f32; 2],
    path: Vec<[f32; 2]>,
}

impl DrawList {
    pub fn new() -> Self {
        Self {
            vtx: Vec::with_capacity(4096),
            idx: Vec::with_capacity(8192),
            cmds: Vec::new(),
            clip_stack: Vec::new(),
            cur_tex: TEX_FONT,
            white_uv: [0.0, 0.0],
            display_size: [0.0, 0.0],
            path: Vec::with_capacity(64),
        }
    }

    pub fn begin_frame(&mut self, display: [f32; 2], white_uv: [f32; 2]) {
        self.vtx.clear();
        self.idx.clear();
        self.cmds.clear();
        self.clip_stack.clear();
        self.path.clear();
        self.white_uv = white_uv;
        self.display_size = display;
        self.cur_tex = TEX_FONT;
        self.clip_stack.push([0.0, 0.0, display[0], display[1]]);
    }

    fn clip(&self) -> [f32; 4] {
        *self.clip_stack.last().unwrap()
    }

    pub fn push_clip_rect(&mut self, min: [f32; 2], max: [f32; 2], intersect: bool) {
        let mut r = [min[0], min[1], max[0], max[1]];
        if intersect {
            let c = self.clip();
            r[0] = r[0].max(c[0]);
            r[1] = r[1].max(c[1]);
            r[2] = r[2].min(c[2]);
            r[3] = r[3].min(c[3]);
        }
        self.clip_stack.push(r);
    }
    pub fn pop_clip_rect(&mut self) {
        if self.clip_stack.len() > 1 {
            self.clip_stack.pop();
        }
    }

    /// Ensure the trailing command matches the current clip+texture; start a new
    /// one otherwise.
    fn cmd_for(&mut self, texture: u8) {
        let clip = self.clip();
        let start = self.idx.len() as u32;
        let reuse = self
            .cmds
            .last()
            .map(|c| c.texture == texture && c.clip == clip && c.idx_offset + c.elem_count == start)
            .unwrap_or(false);
        if !reuse {
            self.cmds.push(DrawCmd {
                clip,
                texture,
                idx_offset: start,
                elem_count: 0,
            });
        }
    }

    #[inline]
    fn prim_reserve(&mut self, texture: u8) -> u32 {
        self.cmd_for(texture);
        self.vtx.len() as u32
    }

    #[inline]
    fn push_idx(&mut self, i: u32) {
        self.idx.push(i);
        if let Some(c) = self.cmds.last_mut() {
            c.elem_count += 1;
        }
    }

    fn add_triangle_raw(&mut self, a: usize, b: usize, c: usize) {
        self.push_idx(a as u32);
        self.push_idx(b as u32);
        self.push_idx(c as u32);
    }

    // -- core primitives -----------------------------------------------------

    /// Fill a convex polygon (fan triangulation), no AA.
    pub fn add_convex_poly_filled(&mut self, points: &[[f32; 2]], col: Color) {
        if points.len() < 3 || (col >> 24) == 0 {
            return;
        }
        let uv = self.white_uv;
        let base = self.prim_reserve(TEX_FONT);
        for p in points {
            self.vtx.push(Vertex {
                pos: *p,
                col,
                uv,
            });
        }
        for i in 2..points.len() {
            self.add_triangle_raw(base as usize, base as usize + i - 1, base as usize + i);
        }
    }

    /// Stroke a polyline as quads, no AA.
    pub fn add_polyline(&mut self, points: &[[f32; 2]], col: Color, closed: bool, thickness: f32) {
        if points.len() < 2 || (col >> 24) == 0 {
            return;
        }
        let uv = self.white_uv;
        let count = if closed { points.len() } else { points.len() - 1 };
        let half = (thickness * 0.5).max(0.5);
        self.cmd_for(TEX_FONT);
        for i in 0..count {
            let p1 = points[i];
            let p2 = points[(i + 1) % points.len()];
            let mut dx = p2[0] - p1[0];
            let mut dy = p2[1] - p1[1];
            let len = (dx * dx + dy * dy).sqrt();
            if len > 0.0 {
                dx /= len;
                dy /= len;
            }
            let nx = dy * half;
            let ny = -dx * half;
            let base = self.vtx.len() as u32;
            self.vtx.push(Vertex { pos: [p1[0] + nx, p1[1] + ny], col, uv });
            self.vtx.push(Vertex { pos: [p2[0] + nx, p2[1] + ny], col, uv });
            self.vtx.push(Vertex { pos: [p2[0] - nx, p2[1] - ny], col, uv });
            self.vtx.push(Vertex { pos: [p1[0] - nx, p1[1] - ny], col, uv });
            self.add_triangle_raw(base as usize, base as usize + 1, base as usize + 2);
            self.add_triangle_raw(base as usize, base as usize + 2, base as usize + 3);
        }
    }

    // -- path building -------------------------------------------------------

    fn path_arc_to(&mut self, center: [f32; 2], radius: f32, a_min: f32, a_max: f32, segments: i32) {
        if radius <= 0.0 {
            self.path.push(center);
            return;
        }
        for i in 0..=segments {
            let a = a_min + (i as f32 / segments as f32) * (a_max - a_min);
            self.path
                .push([center[0] + a.cos() * radius, center[1] + a.sin() * radius]);
        }
    }

    fn path_rect_rounded(&mut self, min: [f32; 2], max: [f32; 2], mut rounding: f32, flags: u32) {
        self.path.clear();
        let w = max[0] - min[0];
        let h = max[1] - min[1];
        rounding = rounding.min(w.abs() * 0.5 - 1.0).min(h.abs() * 0.5 - 1.0);
        if rounding <= 0.0 || flags == 0 {
            self.path.push([min[0], min[1]]);
            self.path.push([max[0], min[1]]);
            self.path.push([max[0], max[1]]);
            self.path.push([min[0], max[1]]);
            return;
        }
        let pi = core::f32::consts::PI;
        let seg = 9;
        let r_tl = if flags & corner::TOP_LEFT != 0 { rounding } else { 0.0 };
        let r_tr = if flags & corner::TOP_RIGHT != 0 { rounding } else { 0.0 };
        let r_br = if flags & corner::BOT_RIGHT != 0 { rounding } else { 0.0 };
        let r_bl = if flags & corner::BOT_LEFT != 0 { rounding } else { 0.0 };
        self.path_arc_to([min[0] + r_tl, min[1] + r_tl], r_tl, pi, pi * 1.5, seg);
        self.path_arc_to([max[0] - r_tr, min[1] + r_tr], r_tr, pi * 1.5, pi * 2.0, seg);
        self.path_arc_to([max[0] - r_br, max[1] - r_br], r_br, 0.0, pi * 0.5, seg);
        self.path_arc_to([min[0] + r_bl, max[1] - r_bl], r_bl, pi * 0.5, pi, seg);
    }

    // -- high-level shapes (ImDrawList API) ----------------------------------

    pub fn add_rect_filled(&mut self, min: [f32; 2], max: [f32; 2], col: Color, rounding: f32, flags: u32) {
        if (col >> 24) == 0 {
            return;
        }
        if rounding <= 0.0 || flags == 0 {
            let uv = self.white_uv;
            let base = self.prim_reserve(TEX_FONT);
            self.vtx.push(Vertex { pos: [min[0], min[1]], col, uv });
            self.vtx.push(Vertex { pos: [max[0], min[1]], col, uv });
            self.vtx.push(Vertex { pos: [max[0], max[1]], col, uv });
            self.vtx.push(Vertex { pos: [min[0], max[1]], col, uv });
            self.add_triangle_raw(base as usize, base as usize + 1, base as usize + 2);
            self.add_triangle_raw(base as usize, base as usize + 2, base as usize + 3);
            return;
        }
        self.path_rect_rounded(min, max, rounding, flags);
        let pts = core::mem::take(&mut self.path);
        self.add_convex_poly_filled(&pts, col);
        self.path = pts;
    }

    pub fn add_rect(&mut self, min: [f32; 2], max: [f32; 2], col: Color, rounding: f32, flags: u32, thickness: f32) {
        if (col >> 24) == 0 {
            return;
        }
        self.path_rect_rounded(min, max, rounding, flags);
        let pts = core::mem::take(&mut self.path);
        self.add_polyline(&pts, col, true, thickness);
        self.path = pts;
    }

    pub fn add_line(&mut self, a: [f32; 2], b: [f32; 2], col: Color, thickness: f32) {
        self.add_polyline(&[a, b], col, false, thickness);
    }

    pub fn add_circle_filled(&mut self, center: [f32; 2], radius: f32, col: Color, segments: i32) {
        if radius <= 0.0 || (col >> 24) == 0 {
            return;
        }
        let segments = segments.clamp(3, 64);
        let mut pts = Vec::with_capacity(segments as usize);
        let tau = core::f32::consts::TAU;
        for i in 0..segments {
            let a = (i as f32 / segments as f32) * tau;
            pts.push([center[0] + a.cos() * radius, center[1] + a.sin() * radius]);
        }
        self.add_convex_poly_filled(&pts, col);
    }

    pub fn add_triangle_filled(&mut self, a: [f32; 2], b: [f32; 2], c: [f32; 2], col: Color) {
        self.add_convex_poly_filled(&[a, b, c], col);
    }

    /// Textured quad (for the GIF). `uv_*` in [0,1].
    pub fn add_image(&mut self, texture: u8, min: [f32; 2], max: [f32; 2], uv_min: [f32; 2], uv_max: [f32; 2], col: Color) {
        let base = self.prim_reserve(texture);
        self.vtx.push(Vertex { pos: [min[0], min[1]], col, uv: [uv_min[0], uv_min[1]] });
        self.vtx.push(Vertex { pos: [max[0], min[1]], col, uv: [uv_max[0], uv_min[1]] });
        self.vtx.push(Vertex { pos: [max[0], max[1]], col, uv: [uv_max[0], uv_max[1]] });
        self.vtx.push(Vertex { pos: [min[0], max[1]], col, uv: [uv_min[0], uv_max[1]] });
        self.add_triangle_raw(base as usize, base as usize + 1, base as usize + 2);
        self.add_triangle_raw(base as usize, base as usize + 2, base as usize + 3);
    }

    /// Push a single pre-computed glyph quad (called by the font code).
    pub fn add_glyph_quad(&mut self, min: [f32; 2], max: [f32; 2], uv_min: [f32; 2], uv_max: [f32; 2], col: Color) {
        let base = self.prim_reserve(TEX_FONT);
        self.vtx.push(Vertex { pos: [min[0], min[1]], col, uv: [uv_min[0], uv_min[1]] });
        self.vtx.push(Vertex { pos: [max[0], min[1]], col, uv: [uv_max[0], uv_min[1]] });
        self.vtx.push(Vertex { pos: [max[0], max[1]], col, uv: [uv_max[0], uv_max[1]] });
        self.vtx.push(Vertex { pos: [min[0], max[1]], col, uv: [uv_min[0], uv_max[1]] });
        self.add_triangle_raw(base as usize, base as usize + 1, base as usize + 2);
        self.add_triangle_raw(base as usize, base as usize + 2, base as usize + 3);
    }
}
