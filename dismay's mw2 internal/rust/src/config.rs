//! INI config persistence. Port of `src/dismay/config.cpp`.
//!
//! Uses the same `WritePrivateProfileStringA` / `GetPrivateProfile*` Win32 API
//! as the original so the on-disk `dismay_config.ini` format is byte-compatible.

#![allow(non_snake_case)]

use crate::functions;
use crate::math::Vec4;
use crate::state::vars;
use crate::win32;

const MAX_PATH: usize = 260;

/// NUL-terminate a string into an owned buffer for the `*A` Win32 calls.
fn cstr(s: &str) -> Vec<u8> {
    let mut v = Vec::with_capacity(s.len() + 1);
    v.extend_from_slice(s.as_bytes());
    v.push(0);
    v
}

/// `config::GetConfigPath` — `<dir of this DLL>\dismay_config.ini`.
pub fn GetConfigPath() -> String {
    unsafe {
        let mut hm: win32::HMODULE = core::ptr::null_mut();
        // Resolve our own module from the address of this function.
        let anchor = GetConfigPath as *const () as *const u8;
        if win32::GetModuleHandleExA(
            win32::GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                | win32::GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            anchor,
            &mut hm,
        ) == 0
        {
            return "dismay_config.ini".to_string();
        }

        let mut path = [0u8; MAX_PATH];
        if win32::GetModuleFileNameA(hm, path.as_mut_ptr(), MAX_PATH as u32) == 0 {
            return "dismay_config.ini".to_string();
        }

        let nul = path.iter().position(|&b| b == 0).unwrap_or(MAX_PATH);
        let mut s = String::from_utf8_lossy(&path[..nul]).into_owned();
        if let Some(slash) = s.rfind(['\\', '/']) {
            s.truncate(slash);
        }
        s.push_str("\\dismay_config.ini");
        s
    }
}

// --- INI primitives ---------------------------------------------------------

fn write_str(section: &str, key: &str, value: &str, file: &[u8]) {
    let s = cstr(section);
    let k = cstr(key);
    let val = cstr(value);
    unsafe {
        win32::WritePrivateProfileStringA(s.as_ptr(), k.as_ptr(), val.as_ptr(), file.as_ptr());
    }
}
fn write_int(section: &str, key: &str, val: i32, file: &[u8]) {
    write_str(section, key, &val.to_string(), file);
}
fn write_float(section: &str, key: &str, val: f32, file: &[u8]) {
    // C++ std::to_string(float) is "%f" — six decimal places.
    write_str(section, key, &format!("{val:.6}"), file);
}
fn write_bool(section: &str, key: &str, val: bool, file: &[u8]) {
    write_str(section, key, if val { "1" } else { "0" }, file);
}

fn read_int(section: &str, key: &str, def: i32, file: &[u8]) -> i32 {
    let s = cstr(section);
    let k = cstr(key);
    unsafe { win32::GetPrivateProfileIntA(s.as_ptr(), k.as_ptr(), def, file.as_ptr()) as i32 }
}
fn read_bool(section: &str, key: &str, def: bool, file: &[u8]) -> bool {
    read_int(section, key, if def { 1 } else { 0 }, file) == 1
}
fn read_float(section: &str, key: &str, def: f32, file: &[u8]) -> f32 {
    let s = cstr(section);
    let k = cstr(key);
    let def_str = cstr(&format!("{def:.6}"));
    let mut buf = [0u8; 32];
    unsafe {
        win32::GetPrivateProfileStringA(
            s.as_ptr(),
            k.as_ptr(),
            def_str.as_ptr(),
            buf.as_mut_ptr(),
            buf.len() as u32,
            file.as_ptr(),
        );
    }
    let nul = buf.iter().position(|&b| b == 0).unwrap_or(buf.len());
    atof(&String::from_utf8_lossy(&buf[..nul]))
}

/// `atof` semantics: parse the leading numeric prefix, 0.0 on failure.
fn atof(s: &str) -> f32 {
    let t = s.trim_start();
    let mut end = 0;
    let bytes = t.as_bytes();
    let mut seen_dot = false;
    let mut seen_exp = false;
    while end < bytes.len() {
        let c = bytes[end];
        match c {
            b'0'..=b'9' => {}
            b'+' | b'-' if end == 0 => {}
            b'+' | b'-' if end > 0 && (bytes[end - 1] | 0x20) == b'e' => {}
            b'.' if !seen_dot && !seen_exp => seen_dot = true,
            b'e' | b'E' if !seen_exp && end > 0 => seen_exp = true,
            _ => break,
        }
        end += 1;
    }
    t[..end].parse::<f32>().unwrap_or(0.0)
}

// --- save / load ------------------------------------------------------------

pub fn Save() {
    let file = cstr(&GetConfigPath());
    let v = vars();

    write_int("Visuals", "FPS", v.framesPerSecond, &file);
    write_float("Visuals", "FOV", v.fieldOfView, &file);
    write_bool("Visuals", "DrawSun", v.noSun, &file);
    write_bool("Visuals", "NoCamoEnabled", v.drawCamo, &file);
    write_bool("Visuals", "NoFogEnabled", v.drawFog, &file);
    write_bool("Visuals", "NoBulletsEnabled", v.drawBullets, &file);
    write_bool("Visuals", "MovieMode", v.movieMode, &file);
    write_bool("Visuals", "ClearGlass", v.clearGlass, &file);
    write_int("Visuals", "Fullbright", v.fullbright, &file);
    write_int("Visuals", "LightMap", v.lightmap, &file);

    write_bool("Misc", "Chat", v.enableTextChat, &file);
    write_bool("Misc", "Mouse11", v.enableMouseOneToOne, &file);
    write_float("Misc", "MapSize", v.mapSize, &file);
    write_bool("Misc", "PingText", v.pingText, &file);

    write_int("UI", "AccentR", (v.accent_color.x * 255.0) as i32, &file);
    write_int("UI", "AccentG", (v.accent_color.y * 255.0) as i32, &file);
    write_int("UI", "AccentB", (v.accent_color.z * 255.0) as i32, &file);
    write_int("UI", "AccentA", (v.accent_color.w * 255.0) as i32, &file);

    write_bool("UI", "EnableCrosshair", v.enableCrosshair, &file);
    write_bool("UI", "CrosshairOutline", v.crosshairOutline, &file);
    write_int("UI", "CrosshairR", (v.crosshair_color.x * 255.0) as i32, &file);
    write_int("UI", "CrosshairG", (v.crosshair_color.y * 255.0) as i32, &file);
    write_int("UI", "CrosshairB", (v.crosshair_color.z * 255.0) as i32, &file);
    write_int("UI", "CrosshairA", (v.crosshair_color.w * 255.0) as i32, &file);
    write_float("UI", "CrosshairGap", v.crosshairGap, &file);
    write_float("UI", "CrosshairLength", v.crosshairLength, &file);
    write_float("UI", "CrosshairThickness", v.crosshairThickness, &file);
    write_float("UI", "CrosshairOutlineThickness", v.crosshairOutlineThickness, &file);
    write_float("UI", "CrosshairScale", v.crosshairScale, &file);
    write_float("UI", "CrosshairLengthScale", v.crosshairLengthScale, &file);
    write_float("UI", "CrosshairGapScale", v.crosshairGapScale, &file);
    write_bool("UI", "CrosshairCenterDot", v.crosshairCenterDot, &file);
    write_bool("UI", "CrosshairTStyle", v.crosshairTStyle, &file);
    write_float("UI", "GunX", v.fcg_gun_x, &file);
    write_float("UI", "GunY", v.fcg_gun_y, &file);
    write_float("UI", "GunZ", v.fcg_gun_z, &file);

    // Flush.
    unsafe {
        win32::WritePrivateProfileStringA(
            core::ptr::null(),
            core::ptr::null(),
            core::ptr::null(),
            file.as_ptr(),
        );
    }
}

pub fn Load() {
    let path = GetConfigPath();
    let file = cstr(&path);

    let path_c = cstr(&path);
    if unsafe { win32::GetFileAttributesA(path_c.as_ptr()) } == win32::INVALID_FILE_ATTRIBUTES {
        return;
    }

    let v = vars();

    v.framesPerSecond = read_int("Visuals", "FPS", 400, &file);
    v.fieldOfView = read_float("Visuals", "FOV", 90.0, &file);
    v.noSun = read_bool("Visuals", "DrawSun", true, &file);
    v.drawCamo = read_bool("Visuals", "NoCamoEnabled", false, &file);
    v.drawFog = read_bool("Visuals", "NoFogEnabled", false, &file);
    v.drawBullets = read_bool("Visuals", "NoBulletsEnabled", false, &file);
    v.movieMode = read_bool("Visuals", "MovieMode", false, &file);
    v.clearGlass = read_bool("Visuals", "ClearGlass", false, &file);
    v.fullbright = read_int("Visuals", "Fullbright", 0, &file);
    v.lightmap = read_int("Visuals", "LightMap", 0, &file);

    v.enableTextChat = read_bool("Misc", "Chat", true, &file);
    v.enableMouseOneToOne = read_bool("Misc", "Mouse11", false, &file);
    v.mapSize = read_float("Misc", "MapSize", 1.0, &file);
    v.pingText = read_bool("Misc", "PingText", true, &file);

    let r = read_int("UI", "AccentR", 255, &file);
    let g = read_int("UI", "AccentG", 255, &file);
    let b = read_int("UI", "AccentB", 255, &file);
    let a = read_int("UI", "AccentA", 255, &file);
    v.accent_color = Vec4::new(r as f32 / 255.0, g as f32 / 255.0, b as f32 / 255.0, a as f32 / 255.0);

    v.enableCrosshair = read_bool("UI", "EnableCrosshair", false, &file);
    v.crosshairOutline = read_bool("UI", "CrosshairOutline", true, &file);
    let cr = read_int("UI", "CrosshairR", 225, &file);
    let cg = read_int("UI", "CrosshairG", 255, &file);
    let cb = read_int("UI", "CrosshairB", 255, &file);
    let ca = read_int("UI", "CrosshairA", 255, &file);
    v.crosshair_color = Vec4::new(
        cr as f32 / 255.0,
        cg as f32 / 255.0,
        cb as f32 / 255.0,
        ca as f32 / 255.0,
    );
    v.crosshairGap = read_float("UI", "CrosshairGap", 1.8, &file);
    v.crosshairLength = read_float("UI", "CrosshairLength", 3.5, &file);
    v.crosshairThickness = read_float("UI", "CrosshairThickness", 1.3, &file);
    v.crosshairOutlineThickness = read_float("UI", "CrosshairOutlineThickness", 1.0, &file);
    v.crosshairScale = read_float("UI", "CrosshairScale", 1.0, &file);
    v.crosshairLengthScale = read_float("UI", "CrosshairLengthScale", 1.0, &file);
    v.crosshairGapScale = read_float("UI", "CrosshairGapScale", 1.0, &file);
    v.crosshairCenterDot = read_bool("UI", "CrosshairCenterDot", false, &file);
    v.crosshairTStyle = read_bool("UI", "CrosshairTStyle", false, &file);
    v.fcg_gun_x = read_float("UI", "GunX", 0.0, &file);
    v.fcg_gun_y = read_float("UI", "GunY", 0.0, &file);
    v.fcg_gun_z = read_float("UI", "GunZ", 0.0, &file);

    ApplyToGame();
}

pub fn ApplyToGame() {
    functions::sendFPSandFOV();
    functions::sendMapSize();
    functions::toggleChat();
    functions::sendFOVMin();
    functions::mouseFix();
    functions::fuckTheSunAway();
    functions::sendNoCamo();
    functions::sendNoFog();
    functions::sendNoBullets();
    functions::sendMovie();
    functions::clearGlass();
    functions::sendPingText();
    functions::fuckTheCrosshairAway();
    functions::sendViewModel();

    let v = vars();
    let light_map_cmd = format!("r_lightMap {};", v.lightmap);
    let fullbright_cmd = format!("r_fullbright {};", v.fullbright);
    if v.fullbright != 0 {
        cbuf(&light_map_cmd);
        cbuf(&fullbright_cmd);
    } else {
        cbuf(&fullbright_cmd);
        cbuf(&light_map_cmd);
    }
}

fn cbuf(s: &str) {
    let mut v = Vec::with_capacity(s.len() + 1);
    v.extend_from_slice(s.as_bytes());
    v.push(0);
    unsafe { crate::game::funcs::Cbuf_AddText(0, v.as_ptr()) }
}

#[cfg(test)]
mod tests {
    use super::atof;

    #[test]
    fn atof_parses_like_c() {
        assert_eq!(atof("90.000000"), 90.0);
        assert_eq!(atof("1.8"), 1.8);
        assert_eq!(atof("3.5xyz"), 3.5); // stops at junk
        assert_eq!(atof("  -0.69"), -0.69); // leading spaces + sign
        assert_eq!(atof("abc"), 0.0); // no number => 0
        assert_eq!(atof(""), 0.0);
        assert_eq!(atof("1000"), 1000.0);
    }
}
