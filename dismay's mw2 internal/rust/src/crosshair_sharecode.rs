//! CS-style crosshair share-code decoder. Port of
//! `src/dismay/crosshair_sharecode.cpp`.

#![allow(non_snake_case)]

use crate::math::Vec4;
use crate::state::vars;

const DICTIONARY: &[u8; 57] = b"ABCDEFGHJKLMNOPQRSTUVWXYZabcdefhijkmnopqrstuvwxyz23456789";
const DICT_LEN: usize = 57;
const PAYLOAD_CHARS: usize = 25;
const DECODED_BYTES: usize = 18;

#[derive(Default, Clone, Copy)]
pub struct CsCrosshairDecoded {
    pub gap: f32,
    pub length: f32,
    pub thickness: f32,
    pub outlineThickness: f32,
    pub red: i32,
    pub green: i32,
    pub blue: i32,
    pub alpha: i32,
    pub colorPreset: i32, // 0=Red,1=Green,2=Yellow,3=Blue,4=Cyan,5=Custom
    pub alphaEnabled: bool,
    pub outlineEnabled: bool,
    pub centerDotEnabled: bool,
    pub tStyleEnabled: bool,
}

fn dict_index(c: u8) -> i32 {
    for (i, &d) in DICTIONARY.iter().enumerate() {
        if d == c {
            return i as i32;
        }
    }
    -1
}

fn bignum_mul_add(buf: &mut [u8; DECODED_BYTES], mul: i32, add: i32) {
    let mut carry = add;
    for i in (0..DECODED_BYTES).rev() {
        let v = buf[i] as i32 * mul + carry;
        buf[i] = (v & 0xFF) as u8;
        carry = v >> 8;
    }
}

/// `DecodeCrosshairShareCode`. `share_code` is the raw (NUL-terminated) buffer.
pub fn decode_crosshair_share_code(share_code: &[u8]) -> Option<CsCrosshairDecoded> {
    // View up to the first NUL, matching C-string semantics.
    let nul = share_code.iter().position(|&b| b == 0).unwrap_or(share_code.len());
    let s = &share_code[..nul];

    let mut p = 0usize;
    while p < s.len() && s[p] == b' ' {
        p += 1;
    }

    // Optional "CSGO" / "csgo" prefix, then optional '-'.
    if s.len() >= p + 4
        && (s[p] | 0x20) == b'c'
        && (s[p + 1] | 0x20) == b's'
        && (s[p + 2] | 0x20) == b'g'
        && (s[p + 3] | 0x20) == b'o'
    {
        p += 4;
        if p < s.len() && s[p] == b'-' {
            p += 1;
        }
    }

    let mut clean = [0u8; 64];
    let mut ci = 0usize;
    while p < s.len() && ci < 63 {
        let c = s[p];
        p += 1;
        if c == b'-' || c == b' ' {
            continue;
        }
        clean[ci] = c;
        ci += 1;
    }

    if ci != PAYLOAD_CHARS {
        return None;
    }
    for &c in &clean[..ci] {
        if dict_index(c) < 0 {
            return None;
        }
    }

    // Reverse the payload in place.
    clean[..ci].reverse();

    let mut buf = [0u8; DECODED_BYTES];
    for &c in &clean[..PAYLOAD_CHARS] {
        let idx = dict_index(c);
        bignum_mul_add(&mut buf, DICT_LEN as i32, idx);
    }

    // Checksum: buf[0] == sum(buf[1..]) & 0xFF
    let mut sum: i32 = 0;
    for &b in &buf[1..DECODED_BYTES] {
        sum += b as i32;
    }
    if buf[0] != (sum & 0xFF) as u8 {
        return None;
    }

    Some(CsCrosshairDecoded {
        gap: (buf[2] as i8) as f32 / 10.0,
        outlineThickness: buf[3] as f32 / 2.0,
        red: buf[4] as i32,
        green: buf[5] as i32,
        blue: buf[6] as i32,
        alpha: buf[7] as i32,
        thickness: buf[12] as f32 / 10.0,
        length: buf[14] as f32 / 10.0,
        colorPreset: (buf[10] & 7) as i32,
        outlineEnabled: (buf[10] & 8) == 8,
        centerDotEnabled: ((buf[13] >> 4) & 1) == 1,
        alphaEnabled: ((buf[13] >> 4) & 4) == 4,
        tStyleEnabled: ((buf[13] >> 4) & 8) == 8,
    })
}

/// `ApplyCsCrosshairToVars`.
pub fn apply_cs_crosshair_to_vars(share_code: &[u8]) -> bool {
    let cs = match decode_crosshair_share_code(share_code) {
        Some(cs) => cs,
        None => return false,
    };

    let (r, g, b) = match cs.colorPreset {
        0 => (1.0, 0.0, 0.0),                                          // Red
        1 => (0.0, 1.0, 0.0),                                          // Green
        2 => (1.0, 1.0, 0.0),                                          // Yellow
        3 => (0.0, 0.0, 1.0),                                          // Blue
        4 => (0.0, 1.0, 1.0),                                          // Cyan
        _ => (cs.red as f32 / 255.0, cs.green as f32 / 255.0, cs.blue as f32 / 255.0), // Custom
    };

    let a = if cs.alphaEnabled {
        cs.alpha as f32 / 255.0
    } else {
        1.0
    };

    let v = vars();
    v.crosshair_color = Vec4::new(r, g, b, a);
    v.crosshairOutline = cs.outlineEnabled;
    v.crosshairGap = cs.gap;
    v.crosshairLength = cs.length;
    v.crosshairThickness = cs.thickness;
    v.crosshairOutlineThickness = cs.outlineThickness;
    v.crosshairCenterDot = cs.centerDotEnabled;
    v.crosshairTStyle = cs.tStyleEnabled;
    v.enableCrosshair = true;
    crate::functions::fuckTheCrosshairAway();

    true
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn dictionary_is_57_unique_chars() {
        assert_eq!(DICTIONARY.len(), DICT_LEN);
        for i in 0..DICT_LEN {
            for j in (i + 1)..DICT_LEN {
                assert_ne!(DICTIONARY[i], DICTIONARY[j], "dup at {i},{j}");
            }
        }
        // 'I', 'g', 'l', '0', '1' are intentionally excluded.
        for c in [b'I', b'g', b'l', b'0', b'1'] {
            assert_eq!(dict_index(c), -1);
        }
    }

    /// Encode 18 big-endian bytes into the 25-char payload the decoder expects.
    /// Inverse of `decode`: produce base-57 big-endian digits, then reverse
    /// (the decoder reverses its input before the bignum step).
    fn encode(buf: &[u8; DECODED_BYTES]) -> Vec<u8> {
        let mut n = *buf;
        let mut digits = [0u8; PAYLOAD_CHARS]; // least-significant first
        for d in digits.iter_mut() {
            // n %= 57, n /= 57  (base-256 big-endian long division)
            let mut rem = 0u32;
            for byte in n.iter_mut() {
                let cur = rem * 256 + *byte as u32;
                *byte = (cur / 57) as u8;
                rem = cur % 57;
            }
            *d = rem as u8;
        }
        // big-endian digits = reverse(little-endian); decoder reverses again, so
        // the on-the-wire payload is the little-endian digit order mapped to dict.
        digits.iter().map(|&d| DICTIONARY[d as usize]).collect()
    }

    #[test]
    fn round_trips_a_constructed_code() {
        let mut buf = [0u8; DECODED_BYTES];
        buf[2] = (-15i8) as u8; // gap -1.5
        buf[3] = 6; // outlineThickness 3.0
        buf[4] = 10;
        buf[5] = 20;
        buf[6] = 30;
        buf[7] = 200; // rgba
        buf[10] = 5 | 8; // preset 5 (custom) + outline flag
        buf[12] = 13; // thickness 1.3
        buf[13] = (1 | 4 | 8) << 4; // centerDot + alpha + tStyle
        buf[14] = 35; // length 3.5
        let sum: i32 = buf[1..].iter().map(|&b| b as i32).sum();
        buf[0] = (sum & 0xFF) as u8;

        let mut code = encode(&buf);
        code.push(0); // NUL-terminate like a C string buffer

        let cs = decode_crosshair_share_code(&code).expect("should decode");
        assert_eq!(cs.gap, -1.5);
        assert_eq!(cs.outlineThickness, 3.0);
        assert_eq!((cs.red, cs.green, cs.blue, cs.alpha), (10, 20, 30, 200));
        assert_eq!(cs.colorPreset, 5);
        assert!(cs.outlineEnabled);
        assert_eq!(cs.thickness, 1.3);
        assert_eq!(cs.length, 3.5);
        assert!(cs.centerDotEnabled && cs.alphaEnabled && cs.tStyleEnabled);
    }

    #[test]
    fn accepts_csgo_prefix_and_dashes() {
        let mut buf = [0u8; DECODED_BYTES];
        buf[4] = 42;
        let sum: i32 = buf[1..].iter().map(|&b| b as i32).sum();
        buf[0] = (sum & 0xFF) as u8;
        let payload = encode(&buf);

        // "CSGO-" prefix + dashes sprinkled in must be ignored.
        let mut wire = b"CSGO-".to_vec();
        for (i, c) in payload.iter().enumerate() {
            wire.push(*c);
            if i % 5 == 4 {
                wire.push(b'-');
            }
        }
        wire.push(0);
        let cs = decode_crosshair_share_code(&wire).expect("decodes with prefix/dashes");
        assert_eq!(cs.red, 42);
    }

    #[test]
    fn rejects_bad_input() {
        assert!(decode_crosshair_share_code(b"\0").is_none()); // empty
        assert!(decode_crosshair_share_code(b"too short\0").is_none());
        assert!(decode_crosshair_share_code(b"IIIIIIIIIIIIIIIIIIIIIIIII\0").is_none()); // 'I' not in dict
    }
}
