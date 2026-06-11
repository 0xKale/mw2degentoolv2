//! Animated GIF (the `lain.gif` shown behind the title). Port of the
//! `ksd::D3D9MemoryGif_*` system — decode once with the pure-Rust `image` crate,
//! pick the current frame by elapsed time. The DX9 upload lives in `dx9.rs`.

use crate::assets;
use crate::state::Racy;
use image::AnimationDecoder;
use std::io::Cursor;

struct Gif {
    frames: Vec<Vec<u8>>, // RGBA8 per frame
    delays_ms: Vec<u32>,
    w: u32,
    h: u32,
    total_ms: u32,
    start_ms: u64,
    failed: bool,
}

static GIF: Racy<Option<Gif>> = Racy::new(None);

fn ensure() -> Option<&'static Gif> {
    let slot = GIF.get();
    if slot.is_none() {
        let decoded = decode();
        *slot = Some(decoded);
    }
    let g = slot.as_ref().unwrap();
    if g.failed || g.frames.is_empty() {
        None
    } else {
        Some(g)
    }
}

fn decode() -> Gif {
    let failed = Gif {
        frames: Vec::new(),
        delays_ms: Vec::new(),
        w: 0,
        h: 0,
        total_ms: 1,
        start_ms: 0,
        failed: true,
    };

    let decoder = match image::codecs::gif::GifDecoder::new(Cursor::new(assets::LAIN_GIF)) {
        Ok(d) => d,
        Err(_) => return failed,
    };
    let frames = match decoder.into_frames().collect_frames() {
        Ok(f) => f,
        Err(_) => return failed,
    };
    if frames.is_empty() {
        return failed;
    }

    let mut bufs = Vec::with_capacity(frames.len());
    let mut delays = Vec::with_capacity(frames.len());
    let mut total = 0u32;
    let (mut w, mut h) = (0u32, 0u32);
    for f in frames {
        let (num, den) = f.delay().numer_denom_ms();
        let d = if den == 0 { 100 } else { num / den.max(1) };
        let d = d.clamp(20, 1000);
        let buf = f.into_buffer();
        w = buf.width();
        h = buf.height();
        delays.push(d);
        total += d;
        bufs.push(buf.into_raw());
    }

    crate::log::log(&format!("gif: decoded {} frames {}x{}", bufs.len(), w, h));
    Gif {
        frames: bufs,
        delays_ms: delays,
        w,
        h,
        total_ms: total.max(1),
        start_ms: unsafe { crate::win32::GetTickCount64() },
        failed: false,
    }
}

/// Current frame as `(rgba, width, height)`, or `None` if the GIF isn't usable.
pub fn current_frame() -> Option<(&'static [u8], u32, u32)> {
    let g = ensure()?;
    let now = unsafe { crate::win32::GetTickCount64() };
    let elapsed = ((now.wrapping_sub(g.start_ms)) % g.total_ms as u64) as u32;
    let mut acc = 0u32;
    let mut idx = 0usize;
    for (i, &d) in g.delays_ms.iter().enumerate() {
        acc += d;
        if elapsed < acc {
            idx = i;
            break;
        }
    }
    Some((g.frames[idx].as_slice(), g.w, g.h))
}

/// Whether the GIF is decoded and drawable.
pub fn available() -> bool {
    ensure().is_some()
}
