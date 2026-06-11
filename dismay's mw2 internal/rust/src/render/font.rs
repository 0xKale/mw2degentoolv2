//! Font atlas + text layout. Pure-Rust replacement for ImGui's font atlas and
//! `ImFont::CalcTextSizeA`/text drawing, built on `ab_glyph`.
//!
//! All fonts are rasterised once into a single shared alpha atlas. Each [`Font`]
//! is baked at a fixed pixel size and scaled per draw call (so one bake serves
//! every size the menu asks for), matching `CalcTextSizeA(size, ...)`.

#![allow(dead_code)]

use super::draw_list::{Color, DrawList};
use crate::assets;
use ab_glyph::{Font as _, FontVec, PxScale, ScaleFont};
use std::collections::HashMap;

const ATLAS_W: u32 = 1024;
const ATLAS_H: u32 = 1024;

#[derive(Clone, Copy)]
struct Glyph {
    uv_min: [f32; 2],
    uv_max: [f32; 2],
    size: [f32; 2],   // px at baked scale
    offset: [f32; 2], // from pen origin (baseline) at baked scale
    advance: f32,
}

pub struct Font {
    glyphs: HashMap<char, Glyph>,
    baked_px: f32,
    ascent: f32,
    descent: f32,
    line_height: f32,
}

impl Font {
    /// Width/height of `text` rendered at `size` (ImGui `CalcTextSize`).
    pub fn calc_text_size(&self, text: &str, size: f32) -> [f32; 2] {
        let k = size / self.baked_px;
        let mut w = 0.0f32;
        for ch in text.chars() {
            if let Some(g) = self.glyphs.get(&ch) {
                w += g.advance;
            }
        }
        [w * k, self.line_height * k]
    }

    /// Draw `text` with its top-left at `pos`, scaled to `size`.
    pub fn draw_text(&self, dl: &mut DrawList, size: f32, pos: [f32; 2], col: Color, text: &str) {
        let k = size / self.baked_px;
        let baseline = pos[1] + self.ascent * k;
        let mut pen_x = pos[0];
        for ch in text.chars() {
            if let Some(g) = self.glyphs.get(&ch) {
                if g.size[0] > 0.0 && g.size[1] > 0.0 {
                    let min = [pen_x + g.offset[0] * k, baseline + g.offset[1] * k];
                    let max = [min[0] + g.size[0] * k, min[1] + g.size[1] * k];
                    dl.add_glyph_quad(min, max, g.uv_min, g.uv_max, col);
                }
                pen_x += g.advance * k;
            }
        }
    }
}

/// Shelf-packed alpha atlas accumulating glyphs from several fonts.
pub struct Fonts {
    pub pixels: Vec<u8>, // ATLAS_W * ATLAS_H, alpha
    pub width: u32,
    pub height: u32,
    pub white_uv: [f32; 2],
    pen_x: u32,
    pen_y: u32,
    row_h: u32,

    pub body: Font,  // Roboto Light
    pub bold: Font,  // Roboto Medium
    pub title: Font, // Morpheus
    pub icons: Font, // Font Awesome solid
}

const ASCII: &str = " !\"#$%&'()*+,-./0123456789:;<=>?@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~";

impl Fonts {
    pub fn build() -> Fonts {
        let mut f = Fonts {
            pixels: vec![0u8; (ATLAS_W * ATLAS_H) as usize],
            width: ATLAS_W,
            height: ATLAS_H,
            white_uv: [0.5 / ATLAS_W as f32, 0.5 / ATLAS_H as f32],
            pen_x: 2,
            pen_y: 0,
            row_h: 1,
            body: Font::empty(),
            bold: Font::empty(),
            title: Font::empty(),
            icons: Font::empty(),
        };
        // White texel for untextured geometry.
        f.pixels[0] = 255;
        f.pixels[1] = 255;
        f.pixels[ATLAS_W as usize] = 255;
        f.pixels[ATLAS_W as usize + 1] = 255;

        f.body = f.bake(assets::ROBOTO_LIGHT, 36.0, ASCII);
        f.bold = f.bake(assets::ROBOTO_MEDIUM, 36.0, ASCII);
        f.title = f.bake(assets::MORPHEUS, 80.0, "DISMAY6 ");
        // Font Awesome icons used by the tab strip (FA6 private-use codepoints).
        let fa: String = ['\u{f05b}', '\u{f013}', '\u{f06e}', '\u{f11c}']
            .iter()
            .collect();
        f.icons = f.bake(assets::FA_SOLID_900, 36.0, &fa);
        f
    }

    fn alloc(&mut self, w: u32, h: u32) -> (u32, u32) {
        if self.pen_x + w + 1 >= self.width {
            self.pen_x = 0;
            self.pen_y += self.row_h + 1;
            self.row_h = 0;
        }
        let (x, y) = (self.pen_x, self.pen_y);
        self.pen_x += w + 1;
        if h > self.row_h {
            self.row_h = h;
        }
        (x, y)
    }

    fn bake(&mut self, bytes: &[u8], px: f32, chars: &str) -> Font {
        let font = match FontVec::try_from_vec(bytes.to_vec()) {
            Ok(f) => f,
            Err(_) => return Font::empty(),
        };
        let scale = PxScale::from(px);
        let scaled = font.as_scaled(scale);
        let ascent = scaled.ascent();
        let descent = scaled.descent();
        let line_height = ascent - descent + scaled.line_gap();

        let mut glyphs = HashMap::new();
        for ch in chars.chars() {
            let gid = font.glyph_id(ch);
            let advance = scaled.h_advance(gid);
            let glyph = gid.with_scale(scale);
            if let Some(outline) = font.outline_glyph(glyph) {
                let bounds = outline.px_bounds();
                let gw = bounds.width().ceil() as u32;
                let gh = bounds.height().ceil() as u32;
                if gw > 0 && gh > 0 && self.pen_y + gh + 1 < self.height {
                    let (ox, oy) = self.alloc(gw, gh);
                    outline.draw(|gx, gy, c| {
                        let px_ = ox + gx;
                        let py_ = oy + gy;
                        if px_ < self.width && py_ < self.height {
                            let idx = (py_ * self.width + px_) as usize;
                            let v = (c * 255.0) as u32;
                            self.pixels[idx] = v.min(255) as u8;
                        }
                    });
                    glyphs.insert(
                        ch,
                        Glyph {
                            uv_min: [ox as f32 / self.width as f32, oy as f32 / self.height as f32],
                            uv_max: [
                                (ox + gw) as f32 / self.width as f32,
                                (oy + gh) as f32 / self.height as f32,
                            ],
                            size: [gw as f32, gh as f32],
                            offset: [bounds.min.x, bounds.min.y],
                            advance,
                        },
                    );
                    continue;
                }
            }
            // No outline (e.g. space) — advance only.
            glyphs.insert(
                ch,
                Glyph {
                    uv_min: [0.0, 0.0],
                    uv_max: [0.0, 0.0],
                    size: [0.0, 0.0],
                    offset: [0.0, 0.0],
                    advance,
                },
            );
        }

        Font {
            glyphs,
            baked_px: px,
            ascent,
            descent,
            line_height,
        }
    }

    /// RGBA8 expansion of the alpha atlas (white, alpha = coverage) for upload.
    pub fn rgba(&self) -> Vec<u8> {
        let mut out = vec![0u8; self.pixels.len() * 4];
        for (i, &a) in self.pixels.iter().enumerate() {
            out[i * 4] = 255;
            out[i * 4 + 1] = 255;
            out[i * 4 + 2] = 255;
            out[i * 4 + 3] = a;
        }
        out
    }
}

impl Font {
    fn empty() -> Font {
        Font {
            glyphs: HashMap::new(),
            baked_px: 16.0,
            ascent: 12.0,
            descent: -4.0,
            line_height: 16.0,
        }
    }
}
