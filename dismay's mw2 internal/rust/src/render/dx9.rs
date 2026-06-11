//! DirectX 9 backend. Pure-Rust port of `imgui_impl_dx9.cpp`: uploads the draw
//! list into dynamic vertex/index buffers and renders it on the game's hooked
//! device with a saved/restored state block, alpha blending, and per-command
//! scissor.
//!
//! Vertices are pre-transformed (`D3DFVF_XYZRHW`) so no projection matrix is
//! needed — screen-pixel coordinates go straight through, with the standard
//! D3D9 −0.5px texel alignment offset.

#![allow(non_snake_case, dead_code)]

use super::draw_list::DrawList;
use super::font::Fonts;
use core::ffi::c_void;
use core::sync::atomic::{AtomicBool, Ordering};
use windows::core::Interface;
use windows::Win32::Foundation::RECT;
use windows::Win32::Graphics::Direct3D9::*;

static FIRST_RENDER: AtomicBool = AtomicBool::new(true);

// FVF / usage / lock / texture-arg bit flags (not all exposed by the metadata).
const D3DFVF_XYZRHW: u32 = 0x004;
const D3DFVF_DIFFUSE: u32 = 0x040;
const D3DFVF_TEX1: u32 = 0x100;
const FVF: u32 = D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1;
const D3DUSAGE_DYNAMIC: u32 = 0x200;
const D3DUSAGE_WRITEONLY: u32 = 0x008;
const D3DLOCK_DISCARD: u32 = 0x2000;
const D3DTA_DIFFUSE: u32 = 0x00000000;
const D3DTA_TEXTURE: u32 = 0x00000002;

#[repr(C)]
#[derive(Clone, Copy)]
struct CustomVertex {
    x: f32,
    y: f32,
    z: f32,
    rhw: f32,
    color: u32, // D3DCOLOR (ARGB)
    u: f32,
    v: f32,
}

#[inline]
fn to_d3dcolor(c: u32) -> u32 {
    // draw_list Color is RGBA (R low) -> D3DCOLOR ARGB (B low).
    let r = c & 0xFF;
    let g = (c >> 8) & 0xFF;
    let b = (c >> 16) & 0xFF;
    let a = (c >> 24) & 0xFF;
    (a << 24) | (r << 16) | (g << 8) | b
}

pub struct Renderer {
    font_tex: Option<IDirect3DTexture9>,
    pub lain_tex: Option<IDirect3DTexture9>,
    lain_w: u32,
    lain_h: u32,
    vb: Option<IDirect3DVertexBuffer9>,
    ib: Option<IDirect3DIndexBuffer9>,
    vb_cap: usize,
    ib_cap: usize,
}

impl Renderer {
    pub const fn new() -> Self {
        Self {
            font_tex: None,
            lain_tex: None,
            lain_w: 0,
            lain_h: 0,
            vb: None,
            ib: None,
            vb_cap: 0,
            ib_cap: 0,
        }
    }

    /// Drop all device-owned resources (call on `Reset`, before the original).
    pub fn invalidate(&mut self) {
        self.font_tex = None;
        self.lain_tex = None;
        self.lain_w = 0;
        self.lain_h = 0;
        self.vb = None;
        self.ib = None;
        self.vb_cap = 0;
        self.ib_cap = 0;
    }

    /// Upload the current GIF frame into a dynamic texture (RGBA -> BGRA).
    pub fn ensure_lain(&mut self, device_ptr: *mut c_void, rgba: &[u8], w: u32, h: u32) {
        if w == 0 || h == 0 || rgba.len() < (w * h * 4) as usize {
            return;
        }
        let raw = device_ptr;
        let device = match unsafe { IDirect3DDevice9::from_raw_borrowed(&raw) } {
            Some(d) => d,
            None => return,
        };
        unsafe {
            if self.lain_tex.is_none() || self.lain_w != w || self.lain_h != h {
                let mut tex = None;
                if device
                    .CreateTexture(
                        w,
                        h,
                        1,
                        D3DUSAGE_DYNAMIC,
                        D3DFMT_A8R8G8B8,
                        D3DPOOL_DEFAULT,
                        &mut tex,
                        core::ptr::null_mut(),
                    )
                    .is_err()
                {
                    return;
                }
                self.lain_tex = tex;
                self.lain_w = w;
                self.lain_h = h;
            }
            let tex = match self.lain_tex.as_ref() {
                Some(t) => t,
                None => return,
            };
            let mut locked = D3DLOCKED_RECT::default();
            if tex
                .LockRect(0, &mut locked, core::ptr::null(), D3DLOCK_DISCARD)
                .is_err()
            {
                return;
            }
            let pitch = locked.Pitch as usize;
            let dst = locked.pBits as *mut u8;
            for y in 0..h as usize {
                let src = &rgba[y * w as usize * 4..];
                let drow = dst.add(y * pitch);
                for x in 0..w as usize {
                    *drow.add(x * 4) = src[x * 4 + 2]; // B
                    *drow.add(x * 4 + 1) = src[x * 4 + 1]; // G
                    *drow.add(x * 4 + 2) = src[x * 4]; // R
                    *drow.add(x * 4 + 3) = src[x * 4 + 3]; // A
                }
            }
            let _ = tex.UnlockRect(0);
        }
    }

    fn ensure_font_texture(&mut self, device: &IDirect3DDevice9, fonts: &Fonts) -> bool {
        if self.font_tex.is_some() {
            return true;
        }
        unsafe {
            let mut tex: Option<IDirect3DTexture9> = None;
            if device
                .CreateTexture(
                    fonts.width,
                    fonts.height,
                    1,
                    0,
                    D3DFMT_A8R8G8B8,
                    D3DPOOL_MANAGED,
                    &mut tex,
                    core::ptr::null_mut(),
                )
                .is_err()
            {
                return false;
            }
            let tex = match tex {
                Some(t) => t,
                None => return false,
            };
            let mut locked = D3DLOCKED_RECT::default();
            if tex.LockRect(0, &mut locked, core::ptr::null(), 0).is_err() {
                return false;
            }
            let rgba = fonts.rgba();
            let pitch = locked.Pitch as usize;
            let dst = locked.pBits as *mut u8;
            for y in 0..fonts.height as usize {
                let src_row = &rgba[y * fonts.width as usize * 4..];
                let dst_row = dst.add(y * pitch);
                core::ptr::copy_nonoverlapping(src_row.as_ptr(), dst_row, fonts.width as usize * 4);
            }
            let _ = tex.UnlockRect(0);
            self.font_tex = Some(tex);
            true
        }
    }

    fn ensure_buffers(&mut self, device: &IDirect3DDevice9, vtx: usize, idx: usize) -> bool {
        unsafe {
            if self.vb.is_none() || self.vb_cap < vtx {
                let cap = vtx + 5000;
                let mut vb = None;
                if device
                    .CreateVertexBuffer(
                        (cap * core::mem::size_of::<CustomVertex>()) as u32,
                        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                        FVF,
                        D3DPOOL_DEFAULT,
                        &mut vb,
                        core::ptr::null_mut(),
                    )
                    .is_err()
                {
                    return false;
                }
                self.vb = vb;
                self.vb_cap = cap;
            }
            if self.ib.is_none() || self.ib_cap < idx {
                let cap = idx + 10000;
                let mut ib = None;
                if device
                    .CreateIndexBuffer(
                        (cap * 4) as u32,
                        D3DUSAGE_DYNAMIC | D3DUSAGE_WRITEONLY,
                        D3DFMT_INDEX32,
                        D3DPOOL_DEFAULT,
                        &mut ib,
                        core::ptr::null_mut(),
                    )
                    .is_err()
                {
                    return false;
                }
                self.ib = ib;
                self.ib_cap = cap;
            }
            true
        }
    }

    pub fn render(&mut self, device_ptr: *mut c_void, dl: &DrawList, fonts: &Fonts, display: [f32; 2]) {
        let first = FIRST_RENDER.swap(false, Ordering::Relaxed);
        if first {
            crate::log::log(&format!(
                "dx9.render: enter, display={}x{} vtx={} idx={} cmds={}",
                display[0],
                display[1],
                dl.vtx.len(),
                dl.idx.len(),
                dl.cmds.len()
            ));
        }
        if display[0] <= 0.0 || display[1] <= 0.0 || dl.idx.is_empty() {
            if first {
                crate::log::log("dx9.render: nothing to draw (empty)");
            }
            return;
        }
        let raw = device_ptr;
        let device = match unsafe { IDirect3DDevice9::from_raw_borrowed(&raw) } {
            Some(d) => d,
            None => {
                if first {
                    crate::log::log("dx9.render: device borrow failed");
                }
                return;
            }
        };

        if !self.ensure_font_texture(device, fonts) {
            if first {
                crate::log::log("dx9.render: ensure_font_texture FAILED");
            }
            return;
        }
        if !self.ensure_buffers(device, dl.vtx.len(), dl.idx.len()) {
            if first {
                crate::log::log("dx9.render: ensure_buffers FAILED");
            }
            return;
        }

        unsafe {
            // Upload vertices (pre-transformed, -0.5px texel offset).
            let vb = self.vb.as_ref().unwrap();
            let ib = self.ib.as_ref().unwrap();
            let mut p: *mut c_void = core::ptr::null_mut();
            if vb
                .Lock(
                    0,
                    (dl.vtx.len() * core::mem::size_of::<CustomVertex>()) as u32,
                    &mut p,
                    D3DLOCK_DISCARD,
                )
                .is_err()
            {
                return;
            }
            let out = p as *mut CustomVertex;
            for (i, v) in dl.vtx.iter().enumerate() {
                *out.add(i) = CustomVertex {
                    x: v.pos[0] - 0.5,
                    y: v.pos[1] - 0.5,
                    z: 0.0,
                    rhw: 1.0,
                    color: to_d3dcolor(v.col),
                    u: v.uv[0],
                    v: v.uv[1],
                };
            }
            let _ = vb.Unlock();

            let mut pi: *mut c_void = core::ptr::null_mut();
            if ib.Lock(0, (dl.idx.len() * 4) as u32, &mut pi, D3DLOCK_DISCARD).is_err() {
                return;
            }
            core::ptr::copy_nonoverlapping(dl.idx.as_ptr(), pi as *mut u32, dl.idx.len());
            let _ = ib.Unlock();

            // Save full device state.
            let state_block = device.CreateStateBlock(D3DSBT_ALL).ok();

            self.setup_render_state(device, vb, ib);

            // Draw each command.
            for cmd in &dl.cmds {
                if cmd.elem_count == 0 {
                    continue;
                }
                let tex = if cmd.texture == super::draw_list::TEX_LAIN {
                    self.lain_tex.as_ref().or(self.font_tex.as_ref())
                } else {
                    self.font_tex.as_ref()
                };
                let base: Option<IDirect3DBaseTexture9> = tex.and_then(|t| t.cast().ok());
                let _ = device.SetTexture(0, base.as_ref());
                let rect = RECT {
                    left: cmd.clip[0] as i32,
                    top: cmd.clip[1] as i32,
                    right: cmd.clip[2] as i32,
                    bottom: cmd.clip[3] as i32,
                };
                let _ = device.SetScissorRect(&rect);
                let hr = device.DrawIndexedPrimitive(
                    D3DPT_TRIANGLELIST,
                    0,
                    0,
                    dl.vtx.len() as u32,
                    cmd.idx_offset,
                    cmd.elem_count / 3,
                );
                if first {
                    crate::log::log(&format!(
                        "dx9.render: DrawIndexedPrimitive -> {:?}, tex_bound={}",
                        hr.is_ok(),
                        base.is_some()
                    ));
                }
            }

            if let Some(sb) = &state_block {
                let _ = sb.Apply();
            }

            if first {
                crate::log::log(&format!(
                    "dx9.render: first frame submitted ({} cmds, state_block={})",
                    dl.cmds.len(),
                    state_block.is_some()
                ));
            }
        }
    }

    fn setup_render_state(
        &self,
        device: &IDirect3DDevice9,
        vb: &IDirect3DVertexBuffer9,
        ib: &IDirect3DIndexBuffer9,
    ) {
        unsafe {
            let _ = device.SetPixelShader(None);
            let _ = device.SetVertexShader(None);
            let _ = device.SetStreamSource(0, vb, 0, core::mem::size_of::<CustomVertex>() as u32);
            let _ = device.SetIndices(ib);
            let _ = device.SetFVF(FVF);

            let rs = |s: D3DRENDERSTATETYPE, v: u32| {
                let _ = device.SetRenderState(s, v);
            };
            rs(D3DRS_CULLMODE, D3DCULL_NONE.0 as u32);
            rs(D3DRS_FILLMODE, D3DFILL_SOLID.0 as u32);
            rs(D3DRS_LIGHTING, 0);
            rs(D3DRS_ZENABLE, 0);
            rs(D3DRS_ALPHABLENDENABLE, 1);
            rs(D3DRS_ALPHATESTENABLE, 0);
            rs(D3DRS_BLENDOP, D3DBLENDOP_ADD.0 as u32);
            rs(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA.0 as u32);
            rs(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA.0 as u32);
            rs(D3DRS_SCISSORTESTENABLE, 1);
            rs(D3DRS_SHADEMODE, D3DSHADE_GOURAUD.0 as u32);
            rs(D3DRS_FOGENABLE, 0);
            rs(D3DRS_RANGEFOGENABLE, 0);
            rs(D3DRS_SPECULARENABLE, 0);
            rs(D3DRS_STENCILENABLE, 0);
            rs(D3DRS_CLIPPING, 1);

            let ts = |t: D3DTEXTURESTAGESTATETYPE, v: u32| {
                let _ = device.SetTextureStageState(0, t, v);
            };
            ts(D3DTSS_COLOROP, D3DTOP_MODULATE.0 as u32);
            ts(D3DTSS_COLORARG1, D3DTA_TEXTURE);
            ts(D3DTSS_COLORARG2, D3DTA_DIFFUSE);
            ts(D3DTSS_ALPHAOP, D3DTOP_MODULATE.0 as u32);
            ts(D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            ts(D3DTSS_ALPHAARG2, D3DTA_DIFFUSE);

            let ss = |t: D3DSAMPLERSTATETYPE, v: u32| {
                let _ = device.SetSamplerState(0, t, v);
            };
            ss(D3DSAMP_MINFILTER, D3DTEXF_LINEAR.0 as u32);
            ss(D3DSAMP_MAGFILTER, D3DTEXF_LINEAR.0 as u32);
            ss(D3DSAMP_ADDRESSU, D3DTADDRESS_CLAMP.0 as u32);
            ss(D3DSAMP_ADDRESSV, D3DTADDRESS_CLAMP.0 as u32);

            // Make sure the game didn't leave color writes off, sRGB on, or a
            // live stage-1 that corrupts our single-stage output.
            rs(D3DRS_COLORWRITEENABLE, 0x0000000F);
            rs(D3DRS_SRGBWRITEENABLE, 0);
            let _ = device.SetTextureStageState(1, D3DTSS_COLOROP, D3DTOP_DISABLE.0 as u32);
            let _ = device.SetTextureStageState(1, D3DTSS_ALPHAOP, D3DTOP_DISABLE.0 as u32);
        }
    }
}
