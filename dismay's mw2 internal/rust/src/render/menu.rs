//! The menu itself — window chrome, title, tab strip, page dispatch — plus the
//! crosshair overlay. Port of `gui.cpp::Render` and `DrawCrosshairOverlay`.

use super::context::{lerp, Ui};
use super::draw_list::{col32, corner};
use super::MENU;
use crate::math::Vec4;
use crate::state::vars;

// Layout constants (from settings.h / gui.cpp).
const MARGIN: f32 = 10.0;
const BODY_W: f32 = 628.0; // menu_interior_body_width_px
const MENU_W: f32 = 648.0; // menu_window_inner_width_px
const MENU_H: f32 = 520.0;
const TAB_STRIP_START_X: f32 = 116.0;
const LEFT_PILL_W: f32 = TAB_STRIP_START_X - 8.0; // 108

// Colors (colors::menu / colors::tabs).
const WINDOW_BG: Vec4 = Vec4::rgb(20, 20, 22);
const GENERAL_CHILD: Vec4 = Vec4::rgb(23, 23, 25);
const TAB_ACTIVE: Vec4 = Vec4::rgb(255, 255, 255);
const TAB_HOVERED: Vec4 = Vec4::rgb(150, 150, 150);
const TAB_INACTIVE: Vec4 = Vec4::rgb(76, 76, 77);

const TAB_LABELS: [&str; 5] = ["Main", "Account", "Host", "Dedigamer", "About"];
const TAB_WIDTHS: [f32; 5] = [80.0, 90.0, 74.0, 102.0, 72.0];
const TAB_ICONS: [char; 5] = ['\u{f05b}', '\u{f06e}', '\u{f013}', '\u{f013}', '\u{f013}'];
const TAB_SPACING: f32 = 4.0;

pub fn draw(ui: &mut Ui) {
    let display = ui.display;
    let win_x = ((display[0] - MENU_W) * 0.5).round();
    let win_y = ((display[1] - MENU_H) * 0.5).round();

    let accent = vars().accent_color;

    // Window background.
    let bg = ui.col(WINDOW_BG);
    ui.dl
        .add_rect_filled([win_x, win_y], [win_x + MENU_W, win_y + MENU_H], bg, 8.0, corner::ALL);

    // Header band: left title pill + right tab bar, at window + (10, 10).
    let cx0 = win_x + MARGIN;
    let cy0 = win_y + MARGIN;
    let header = ui.col(GENERAL_CHILD);
    ui.dl
        .add_rect_filled([cx0, cy0], [cx0 + LEFT_PILL_W, cy0 + 60.0], header, 10.0, corner::ALL);
    ui.dl.add_rect_filled(
        [cx0 + TAB_STRIP_START_X, cy0],
        [cx0 + BODY_W, cy0 + 60.0],
        header,
        10.0,
        corner::ALL,
    );

    // Lain GIF behind the title (clipped to the pill, ~30% alpha).
    if super::gif::available() {
        let pill_min = [cx0, cy0];
        let pill_max = [cx0 + LEFT_PILL_W, cy0 + 60.0];
        ui.dl.push_clip_rect(pill_min, pill_max, true);
        let a = (0.30 * ui.alpha * 255.0) as u8;
        ui.dl.add_image(
            super::draw_list::TEX_LAIN,
            pill_min,
            pill_max,
            [0.0, 0.0],
            [1.0, 1.0],
            col32(255, 255, 255, a),
        );
        ui.dl.pop_clip_rect();
    }

    draw_title(ui, cx0, cy0, accent);
    draw_tabs(ui, cx0, cy0, accent);

    // Body at window + (10, 80), interior 628 x 440 — scrollable.
    let body_min = [cx0, win_y + 80.0];
    let body_max = [cx0 + BODY_W, body_min[1] + 440.0];
    let tab = MENU.get().tab_count.clamp(0, 4);
    MENU.get().active_tab_count = tab;

    {
        let m = MENU.get();
        if ui.input.hovers(body_min, body_max) {
            m.scroll_y[tab as usize] -= ui.input.wheel * 40.0;
        }
        let content_h = if tab == 0 {
            super::widgets::main_content_height()
        } else {
            200.0
        };
        let max_scroll = (content_h - 440.0).max(0.0);
        m.scroll_y[tab as usize] = m.scroll_y[tab as usize].clamp(0.0, max_scroll);
    }
    let scroll = MENU.get().scroll_y[tab as usize];
    let origin = [body_min[0] + 11.0, body_min[1] + 8.0 - scroll];

    ui.dl.push_clip_rect(body_min, body_max, true);
    match tab {
        0 => super::widgets::render_main(ui, origin),
        _ => super::widgets::page_placeholder(ui, origin, TAB_LABELS[tab as usize]),
    }
    ui.dl.pop_clip_rect();
}

fn draw_title(ui: &mut Ui, cx0: f32, cy0: f32, accent: Vec4) {
    // Morpheus "DISMAY", scaled to fit the pill, centered.
    let base = 34.0;
    let natural = ui.fonts.title.calc_text_size("DISMAY", base)[0];
    let size = if natural > LEFT_PILL_W - 10.0 {
        base * (LEFT_PILL_W - 10.0) / natural
    } else {
        base
    };
    let tsz = ui.fonts.title.calc_text_size("DISMAY", size);
    let tx = cx0 + LEFT_PILL_W * 0.5 - tsz[0] * 0.5;
    let ty = cy0 + (60.0 - tsz[1]) * 0.5;
    let core = Vec4::new(
        lerp(1.0, accent.x, 0.12),
        lerp(1.0, accent.y, 0.12),
        lerp(1.0, accent.z, 0.12),
        1.0,
    );
    let col = ui.col(core);
    ui.fonts.title.draw_text(&mut ui.dl, size, [tx, ty], col, "DISMAY");
}

fn draw_tabs(ui: &mut Ui, cx0: f32, cy0: f32, accent: Vec4) {
    // Cumulative offsets for the indicator.
    let mut offsets = [0.0f32; 5];
    for i in 1..5 {
        offsets[i] = offsets[i - 1] + TAB_WIDTHS[i - 1] + TAB_SPACING;
    }

    let selected = MENU.get().tab_count.clamp(0, 4);

    // Tab labels + click handling. Boxes start at child + (116, 12), height 40.
    let mut x = cx0 + TAB_STRIP_START_X;
    let y = cy0 + 12.0;
    for i in 0..5 {
        let w = TAB_WIDTHS[i];
        let min = [x, y];
        let max = [x + w, y + 40.0];
        let (hovered, pressed) = ui.button_behavior(min, max);
        if pressed {
            MENU.get().tab_count = i as i32;
        }
        let col = if selected == i as i32 {
            TAB_ACTIVE
        } else if hovered {
            TAB_HOVERED
        } else {
            TAB_INACTIVE
        };
        let c = ui.col(col);

        // Icon + label, vertically centered.
        let icon = TAB_ICONS[i];
        let mut s = String::new();
        s.push(icon);
        let isz = ui.fonts.icons.calc_text_size(&s, 16.0);
        let label = TAB_LABELS[i];
        let lsz = ui.fonts.bold.calc_text_size(label, 16.0);
        let total = isz[0] + 6.0 + lsz[0];
        let ix = x + (w - total) * 0.5;
        let iy = y + (40.0 - isz[1]) * 0.5;
        ui.fonts.icons.draw_text(&mut ui.dl, 16.0, [ix, iy], c, &s);
        let ly = y + (40.0 - lsz[1]) * 0.5;
        ui.fonts
            .bold
            .draw_text(&mut ui.dl, 16.0, [ix + isz[0] + 6.0, ly], c, label);

        x += w + TAB_SPACING;
    }

    // Animated underline indicator beneath the active label.
    let target = offsets[selected as usize];
    let m = MENU.get();
    m.anim_tab = lerp(m.anim_tab, target, (ui.dt * 15.0).min(1.0));
    let text_offset = 37.0;
    let ind_x = cx0 + TAB_STRIP_START_X + text_offset + m.anim_tab;
    let ind_w = ui.fonts.bold.calc_text_size(TAB_LABELS[selected as usize], 16.0)[0];
    let acc = ui.col(accent);
    ui.dl.add_rect_filled(
        [ind_x, cy0 + 57.0],
        [ind_x + ind_w, cy0 + 60.0],
        acc,
        2.0,
        corner::TOP,
    );
}

/// Port of `functions::DrawCrosshairOverlay` onto the background of the frame.
pub fn draw_crosshair_overlay(ui: &mut Ui) {
    let v = vars();
    if !v.enableCrosshair {
        return;
    }
    let cx = ui.display[0] * 0.5;
    let cy = ui.display[1] * 0.5;

    let scale = (ui.display[1] / 480.0) * v.crosshairScale;
    let th = v.crosshairThickness * scale;
    let gap = v.crosshairGap * scale * v.crosshairGapScale;
    let len = v.crosshairLength * scale * v.crosshairLengthScale;
    let ol_th = v.crosshairOutlineThickness * scale;
    let inner = gap;
    let outer = gap + len;

    let t_style = v.crosshairTStyle;
    let draw_arms = |ui: &mut Ui, c: u32, t: f32| {
        ui.dl.add_line([cx - outer, cy], [cx - inner, cy], c, t);
        ui.dl.add_line([cx + inner, cy], [cx + outer, cy], c, t);
        ui.dl.add_line([cx, cy + inner], [cx, cy + outer], c, t);
        if !t_style {
            ui.dl.add_line([cx, cy - outer], [cx, cy - inner], c, t);
        }
    };

    if v.crosshairOutline {
        draw_arms(ui, col32(0, 0, 0, 255), th + ol_th);
    }
    let cc = ui.col(v.crosshair_color);
    draw_arms(ui, cc, th);

    if v.crosshairCenterDot {
        let mut dot_r = th * 0.5;
        if dot_r < 1.0 {
            dot_r = 1.0;
        }
        if v.crosshairOutline {
            ui.dl
                .add_circle_filled([cx, cy], dot_r + ol_th * 0.5, col32(0, 0, 0, 255), 16);
        }
        let cc = ui.col(v.crosshair_color);
        ui.dl.add_circle_filled([cx, cy], dot_r, cc, 16);
    }
}
