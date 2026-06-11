//! Embedded binary assets, extracted verbatim from the C++ `framework/fonts.h`,
//! `ext/fonts/iconsfontawesome/fa.h`, and `framework/images.h` byte arrays.
//! (Byte-identical to the originals; see `assets/*.ttf` / `*.gif`.)

/// `morpheus[68044]` — the big title font.
pub static MORPHEUS: &[u8] = include_bytes!("../assets/morpheus.ttf");
/// `robotLight[162636]` — Roboto Light (UI body).
pub static ROBOTO_LIGHT: &[u8] = include_bytes!("../assets/roboto_light.ttf");
/// `robotMedium[160696]` — Roboto Medium (UI bold).
pub static ROBOTO_MEDIUM: &[u8] = include_bytes!("../assets/roboto_medium.ttf");
/// `freesolid900[414664]` — Font Awesome 6 Solid (icons).
pub static FA_SOLID_900: &[u8] = include_bytes!("../assets/fa_solid_900.ttf");
/// `lain[746364]` — the animated GIF shown in the title pill.
pub static LAIN_GIF: &[u8] = include_bytes!("../assets/lain.gif");
