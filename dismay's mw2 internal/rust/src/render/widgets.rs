//! Custom widgets (port of `framework/*`) + the per-tab pages (`menu/pages/*`),
//! rebuilt on the pure-Rust draw list. Visuals mirror the originals: panels with
//! a header bar + icon + title, toggle-switch checkboxes, label-left/track-right
//! sliders, and rounded buttons.

use super::context::Ui;
use super::draw_list::corner;
use super::MENU;
use crate::math::Vec4;
use crate::state::vars;

// -- colors (settings.h) -----------------------------------------------------
const CHILD_BG: Vec4 = Vec4::rgb(23, 23, 25);
const CHILD_TOP: Vec4 = Vec4::rgb(25, 25, 27);
const TXT_ACTIVE: Vec4 = Vec4::rgb(255, 255, 255);
const TXT_HOVERED: Vec4 = Vec4::rgb(150, 150, 150);
const TXT_INACTIVE: Vec4 = Vec4::rgb(76, 76, 77);
const CB_BG_ACTIVE: Vec4 = Vec4::rgb(37, 37, 39);
const CB_BG_INACTIVE: Vec4 = Vec4::rgb(35, 35, 37);
const CIRCLE_INACTIVE: Vec4 = Vec4::rgb(44, 44, 46);
const SLIDER_INACTIVE: Vec4 = Vec4::rgb(35, 35, 37);
const BTN_BG: Vec4 = Vec4::rgb(27, 27, 29);
const BTN_BG_SELECTED: Vec4 = Vec4::rgb(36, 38, 42);
const BTN_OUTLINE_SELECTED: Vec4 = Vec4::rgb(52, 54, 58);
const INPUT_BG: Vec4 = Vec4::rgb(27, 27, 29);

fn fnv(s: &str) -> u64 {
    let mut h = 0xcbf2_9ce4_8422_2325u64;
    for b in s.bytes() {
        h ^= b as u64;
        h = h.wrapping_mul(0x0000_0100_0000_01b3);
    }
    h
}

fn fmt_float(v: f32, fmt: &str) -> String {
    match fmt {
        "%.0f" => format!("{v:.0}"),
        "%.1f" => format!("{v:.1}"),
        "%.2f" => format!("{v:.2}"),
        _ => format!("{v:.3}"),
    }
}

impl<'a> Ui<'a> {
    /// `ksd::BeginChild` — draw the panel chrome and position the cursor inside.
    pub fn begin_panel(&mut self, pos: [f32; 2], width: f32, content_height: f32, icon: char, title: &str) {
        let outer_h = content_height + 50.0;
        let bg = self.col(CHILD_BG);
        self.dl
            .add_rect_filled(pos, [pos[0] + width, pos[1] + outer_h], bg, 7.0, corner::ALL);
        let top = self.col(CHILD_TOP);
        self.dl
            .add_rect_filled(pos, [pos[0] + width, pos[1] + 40.0], top, 7.0, corner::TOP);

        let acc = self.col(vars().accent_color);
        let mut ic = String::new();
        ic.push(icon);
        self.fonts
            .icons
            .draw_text(&mut self.dl, 18.0, [pos[0] + 11.0, pos[1] + 13.0], acc, &ic);
        self.fonts
            .bold
            .draw_text(&mut self.dl, 18.0, [pos[0] + 38.0, pos[1] + 12.0], acc, title);

        self.cursor = [pos[0] + 20.0, pos[1] + 50.0];
        self.window_w = width;
    }

    pub fn dummy(&mut self, h: f32) {
        self.cursor[1] += h;
    }

    pub fn text(&mut self, label: &str) {
        let c = self.col(TXT_ACTIVE);
        let p = [self.cursor[0] - 9.0, self.cursor[1] + 3.5];
        self.fonts.bold.draw_text(&mut self.dl, 17.0, p, c, label);
        self.cursor[1] += 20.0;
    }

    /// `ksd::Checkbox` — toggle switch, label on the left.
    pub fn checkbox(&mut self, label: &str, value: &mut bool) -> bool {
        let origin = self.cursor;
        let track_w = (self.window_w - 46.0).max(40.0);
        let row_h = 24.0;
        let (hovered, pressed) =
            self.button_behavior(origin, [origin[0] + 12.0 + track_w, origin[1] + row_h]);
        if pressed {
            *value = !*value;
        }

        let tray_min = [origin[0] + track_w - 20.0, origin[1]];
        let tray_max = [origin[0] + track_w + 16.0, origin[1] + 19.5];
        let bgc = self.col(if *value { CB_BG_ACTIVE } else { CB_BG_INACTIVE });
        self.dl.add_rect_filled(tray_min, tray_max, bgc, 100.0, corner::ALL);

        let interp = if *value { -13.0 } else { 0.0 };
        let center = [origin[0] + track_w - 8.5 - interp, origin[1] + 9.5];
        let circ = self.col(if *value { vars().accent_color } else { CIRCLE_INACTIVE });
        self.dl.add_circle_filled(center, 6.0, circ, 16);

        let tcol = if *value {
            TXT_ACTIVE
        } else if hovered {
            TXT_HOVERED
        } else {
            TXT_INACTIVE
        };
        let tc = self.col(tcol);
        self.fonts
            .bold
            .draw_text(&mut self.dl, 17.0, [origin[0] - 9.0, origin[1] + 3.5], tc, label);

        self.cursor[1] += row_h + 5.0;
        pressed
    }

    /// `ksd::Button`.
    pub fn button(&mut self, label: &str, w: f32, h: f32, selected: bool) -> bool {
        let pos = self.cursor;
        let bb_min = [pos[0] - 10.0, pos[1]];
        let bb_max = [pos[0] + w, pos[1] + h];
        let (hovered, pressed) = self.button_behavior(bb_min, bb_max);

        let fill = self.col(if selected { BTN_BG_SELECTED } else { BTN_BG });
        self.dl.add_rect_filled(bb_min, bb_max, fill, 4.0, corner::ALL);
        let outline = self.col(if selected { BTN_OUTLINE_SELECTED } else { BTN_BG });
        self.dl.add_rect(bb_min, bb_max, outline, 4.0, corner::ALL, 1.0);

        let tcol = if hovered || selected { TXT_ACTIVE } else { TXT_INACTIVE };
        let tc = self.col(tcol);
        let tsz = self.fonts.body.calc_text_size(label, 17.0);
        let tx = bb_min[0] + ((w + 10.0) - tsz[0]) * 0.5;
        let ty = pos[1] + (h - tsz[1]) * 0.5;
        self.fonts.body.draw_text(&mut self.dl, 17.0, [tx, ty], tc, label);

        self.cursor[1] += h + 4.0;
        pressed
    }

    fn slider_core(&mut self, label: &str, frac_in: f32) -> (f32, bool) {
        // returns (new_frac, changed) — caller maps to value.
        let origin = self.cursor;
        let content_w = self.window_w - 35.0;
        let track_lead = (content_w - 140.0).max(40.0);
        let track_min = [origin[0] + track_lead, origin[1] + 17.0 + 6.0];
        let track_max = [origin[0] + content_w, track_min[1] + 6.0];
        let id = fnv(label);

        let hovered = self.input.hovers(track_min, track_max);
        let m = MENU.get();
        if hovered && self.input.clicked[0] {
            m.active_id = id;
        }
        if !self.input.down[0] && m.active_id == id {
            m.active_id = 0;
        }
        let active = m.active_id == id;

        let mut frac = frac_in.clamp(0.0, 1.0);
        let mut changed = false;
        if active {
            let t = ((self.input.mouse_pos[0] - track_min[0]) / (track_max[0] - track_min[0]))
                .clamp(0.0, 1.0);
            if (t - frac).abs() > f32::EPSILON {
                frac = t;
                changed = true;
            }
        }

        let bgc = self.col(SLIDER_INACTIVE);
        self.dl.add_rect_filled(track_min, track_max, bgc, 5.0, corner::ALL);
        let acc_w = frac * (track_max[0] - track_min[0]);
        if acc_w > 0.5 {
            let accc = self.col(vars().accent_color);
            self.dl
                .add_rect_filled(track_min, [track_min[0] + acc_w, track_max[1]], accc, 5.0, corner::ALL);
        }

        let tcol = if active {
            TXT_ACTIVE
        } else if hovered {
            TXT_HOVERED
        } else {
            TXT_INACTIVE
        };
        let tc = self.col(tcol);
        self.fonts
            .bold
            .draw_text(&mut self.dl, 17.0, [origin[0] - 9.0, origin[1] + 3.5], tc, label);
        (frac, changed)
    }

    pub fn slider_float(&mut self, label: &str, value: &mut f32, min: f32, max: f32, fmt: &str) -> bool {
        let frac_in = if max > min { (*value - min) / (max - min) } else { 0.0 };
        let (frac, changed) = self.slider_core(label, frac_in);
        if changed {
            *value = min + frac * (max - min);
        }
        // value text (right-aligned on the label row)
        let origin = self.cursor;
        let content_w = self.window_w - 35.0;
        let s = fmt_float(*value, fmt);
        let vsz = self.fonts.bold.calc_text_size(&s, 17.0);
        let tc = self.col(TXT_HOVERED);
        self.fonts.bold.draw_text(
            &mut self.dl,
            17.0,
            [origin[0] + content_w - vsz[0], origin[1] + 3.5],
            tc,
            &s,
        );
        self.cursor[1] += 17.0 + 6.0 + 6.0 + 3.0;
        changed
    }

    pub fn slider_int(&mut self, label: &str, value: &mut i32, min: i32, max: i32) -> bool {
        let frac_in = if max > min {
            (*value - min) as f32 / (max - min) as f32
        } else {
            0.0
        };
        let (frac, changed) = self.slider_core(label, frac_in);
        if changed {
            *value = min + (frac * (max - min) as f32).round() as i32;
        }
        let origin = self.cursor;
        let content_w = self.window_w - 35.0;
        let s = format!("{value}");
        let vsz = self.fonts.bold.calc_text_size(&s, 17.0);
        let tc = self.col(TXT_HOVERED);
        self.fonts.bold.draw_text(
            &mut self.dl,
            17.0,
            [origin[0] + content_w - vsz[0], origin[1] + 3.5],
            tc,
            &s,
        );
        self.cursor[1] += 17.0 + 6.0 + 6.0 + 3.0;
        changed
    }

    /// Display-only input box (text editing is a later pass).
    pub fn input_display(&mut self, label: &str, value: &str) {
        let origin = self.cursor;
        let content_w = self.window_w - 30.0;
        if !label.starts_with("##") {
            let tc = self.col(TXT_HOVERED);
            self.fonts
                .bold
                .draw_text(&mut self.dl, 16.0, [origin[0] - 9.0, origin[1]], tc, label);
        }
        let box_y = origin[1] + 20.0;
        let bg = self.col(INPUT_BG);
        self.dl.add_rect_filled(
            [origin[0] - 9.0, box_y],
            [origin[0] - 9.0 + content_w, box_y + 26.0],
            bg,
            4.0,
            corner::ALL,
        );
        let tc = self.col(TXT_ACTIVE);
        self.fonts
            .body
            .draw_text(&mut self.dl, 15.0, [origin[0], box_y + 5.0], tc, value);
        self.cursor[1] += 50.0;
    }
}

// ---------------------------------------------------------------------------
// Pages
// ---------------------------------------------------------------------------

/// `Cbuf_AddText(0, text)` for a NUL-terminated command.
fn cbuf(text: &[u8]) {
    unsafe { crate::game::funcs::Cbuf_AddText(0, text.as_ptr()) }
}

/// `menu_pages::RenderMainPage`.
pub fn render_main(ui: &mut Ui, origin: [f32; 2]) {
    let left_x = origin[0] - 10.0;
    let right_x = origin[0] + 309.0;
    let left_w = 309.0;
    let right_w = 307.0;
    let v = vars();

    let main_content = if v.enableCustomPort { 575.0 } else { 535.0 };
    let main_outer = main_content + 50.0;

    // -- left column: Main --
    ui.begin_panel([left_x, origin[1]], left_w, main_content, '\u{f05b}', "Main");
    if ui.checkbox("Enable Text Chat", &mut v.enableTextChat) {
        crate::functions::toggleChat();
    }
    ui.checkbox("Enable Mouse 1:1", &mut v.enableMouseOneToOne);
    if ui.checkbox("Iron Sight Intervention", &mut v.ironSightIntervention) {
        crate::functions::doIronSight();
    }
    ui.input_display("Sensitivity", &fmt_float(v.mouseSensitivity, "%.3f"));
    if ui.button("Send Sensitivity", 280.0, 30.0, false) {
        crate::functions::writeSensitivity(v.mouseSensitivity);
    }
    ui.slider_int("Frames Per Second", &mut v.framesPerSecond, 30, 1000);
    ui.slider_float("Field Of View", &mut v.fieldOfView, 65.0, 120.0, "%.0f");
    ui.slider_float("Map Size", &mut v.mapSize, 1.0, 2.0, "%.3f");
    ui.dummy(10.0);
    if ui.button("Disconnect", 280.0, 30.0, false) {
        cbuf(b"disconnect\0");
    }
    ui.text("Console");
    ui.input_display("##console", cstr_str(&v.console));
    if ui.button("Send Console", 280.0, 30.0, false) {
        cbuf(&nul_terminated(cstr_str(&v.console)));
    }
    ui.checkbox("Enable DLC?", &mut v.enableDLC);
    if ui.checkbox("Enable Custom Port?", &mut v.enableCustomPort) {
        crate::functions::sendCustomPort();
    }
    if v.enableCustomPort {
        ui.input_display("Custom Port", &format!("{}", v.customPort));
    }
    if ui.button("Force Team Change", 280.0, 30.0, false) {
        crate::functions::forceTeamChange();
    }
    let mut watermark = features_watermark();
    ui.checkbox("Watermark", &mut watermark);

    // -- left column: Crosshair --
    let crosshair_y = origin[1] + main_outer + 10.0;
    ui.begin_panel([left_x, crosshair_y], left_w, 230.0, '\u{f05b}', "Crosshair");
    if ui.checkbox("Enable Crosshair", &mut v.enableCrosshair) {
        crate::functions::fuckTheCrosshairAway();
    }
    ui.checkbox("Crosshair Outline", &mut v.crosshairOutline);
    ui.checkbox("Center Dot", &mut v.crosshairCenterDot);
    ui.checkbox("T-Style", &mut v.crosshairTStyle);
    ui.slider_float("Crosshair Scale", &mut v.crosshairScale, 0.5, 2.0, "%.2f");
    ui.slider_float("Crosshair Length", &mut v.crosshairLengthScale, 0.5, 3.0, "%.2f");
    ui.slider_float("Crosshair Gap", &mut v.crosshairGapScale, 0.5, 3.0, "%.2f");

    // -- right column: Toggles --
    ui.begin_panel([right_x, origin[1]], right_w, 230.0, '\u{f11c}', "Toggles");
    if ui.checkbox("Draw Sun", &mut v.noSun) {
        crate::functions::fuckTheSunAway();
    }
    if ui.checkbox("Draw Camos", &mut v.drawCamo) {
        crate::functions::sendNoCamo();
    }
    if ui.checkbox("Draw Fog", &mut v.drawFog) {
        crate::functions::sendNoFog();
    }
    if ui.checkbox("Draw Bullets", &mut v.drawBullets) {
        crate::functions::sendNoBullets();
    }
    if ui.checkbox("Movie Mode", &mut v.movieMode) {
        crate::functions::sendMovie();
    }
    if ui.checkbox("Clear Glass", &mut v.clearGlass) {
        crate::functions::clearGlass();
    }
    if ui.checkbox("Ping Text", &mut v.pingText) {
        crate::functions::sendPingText();
    }

    // -- right column: Fullbright --
    ui.begin_panel([right_x, origin[1] + 290.0], right_w, 190.0, '\u{f013}', "Fullbright");
    if ui.button("Invert", 280.0, 30.0, v.fullbright == 0 && v.lightmap == 0) {
        v.fullbright = 0;
        v.lightmap = 0;
        cbuf(b"r_fullbright 0;r_lightMap 0;\0");
    }
    if ui.button("Normal", 280.0, 30.0, v.fullbright == 0 && v.lightmap == 1) {
        v.fullbright = 0;
        v.lightmap = 1;
        cbuf(b"r_fullbright 0;r_lightMap 1;\0");
    }
    if ui.button("Super", 280.0, 30.0, v.fullbright == 0 && v.lightmap == 2) {
        v.fullbright = 0;
        v.lightmap = 2;
        cbuf(b"r_fullbright 0;r_lightMap 2;\0");
    }
    if ui.button("Slight", 280.0, 30.0, v.fullbright == 0 && v.lightmap == 3) {
        v.fullbright = 0;
        v.lightmap = 3;
        cbuf(b"r_fullbright 0;r_lightMap 3;\0");
    }
    if ui.button("Dullish", 280.0, 30.0, v.fullbright == 1 && v.lightmap == 0) {
        v.fullbright = 1;
        v.lightmap = 0;
        cbuf(b"r_fullbright 1;r_lightMap 1;\0");
    }

    // -- right column: View Model --
    ui.begin_panel([right_x, origin[1] + 540.0], right_w, 155.0, '\u{f05b}', "View Model");
    if ui.slider_float("Gun X", &mut v.fcg_gun_x, -10.0, 10.0, "%.2f") {
        crate::functions::sendViewModel();
    }
    if ui.slider_float("Gun Y", &mut v.fcg_gun_y, -10.0, 10.0, "%.2f") {
        crate::functions::sendViewModel();
    }
    if ui.slider_float("Gun Z", &mut v.fcg_gun_z, -10.0, 10.0, "%.2f") {
        crate::functions::sendViewModel();
    }
    if ui.button("Reset View Model", 280.0, 30.0, false) {
        v.fcg_gun_x = 0.0;
        v.fcg_gun_y = 0.0;
        v.fcg_gun_z = 0.0;
        crate::functions::sendViewModel();
    }
}

/// Total content height of a page (for scroll clamping).
pub fn main_content_height() -> f32 {
    let v = vars();
    let main_outer = (if v.enableCustomPort { 575.0 } else { 535.0 }) + 50.0;
    // left column is the tallest: Main + gap + Crosshair
    main_outer + 10.0 + (230.0 + 50.0)
}

/// Placeholder for tabs not yet ported.
pub fn page_placeholder(ui: &mut Ui, origin: [f32; 2], name: &str) {
    ui.begin_panel([origin[0] - 10.0, origin[1]], 309.0, 120.0, '\u{f013}', name);
    let c = ui.col(Vec4::rgb(150, 150, 150));
    ui.fonts.body.draw_text(
        &mut ui.dl,
        15.0,
        [origin[0], origin[1] + 60.0],
        c,
        "this tab is being ported next",
    );
}

// -- helpers for borrowing odd globals --------------------------------------

fn features_watermark() -> bool {
    // `features::watermark` default true; standalone bool since the watermark
    // render isn't ported yet.
    true
}

fn cstr_str(buf: &[u8]) -> &str {
    let n = buf.iter().position(|&b| b == 0).unwrap_or(buf.len());
    core::str::from_utf8(&buf[..n]).unwrap_or("")
}

fn nul_terminated(s: &str) -> Vec<u8> {
    let mut v = s.as_bytes().to_vec();
    v.push(0);
    v
}
