# Rust port — status

A **100% Rust** port of *dismay's mw2 internal* — no C or C++ anywhere, no FFI
into the C++ ImGui. The original C++ tree is left in place for reference; this
crate stands alone.

Injected into **MW2.exe (a 32-bit process)**, so it builds as a 32-bit
(`i686-pc-windows-msvc`) `cdylib` — the default target (see `.cargo/config.toml`).

## Dependencies (all pure Rust)
- `windows` 0.62 — Win32 + Direct3D 9 (metadata-generated bindings, no C).
- *(hooking)* VMT vtable swap, hand-written — no MinHook, no `retour`
  (`retour` is broken on 32-bit: it hardcodes the `win64` ABI).
- Planned for the renderer: `ab_glyph`/`fontdue` (glyph rasterising, pure Rust)
  and `image` (GIF decode, pure Rust). Not added yet.

## How to build / test
```sh
cd rust
cargo build            # -> target/i686-pc-windows-msvc/debug/mw2degentool.dll (PE machine 0x14c)
cargo build --release  # LTO, panic=abort
cargo test             # 7 pure-logic tests
```
Debug + release are **warning-free**; tests pass.

## Done ✅ (compiles; pure logic is unit-tested)

**Game-logic core** — faithful 1:1, verified:

| Rust | C++ | Notes |
|---|---|---|
| `game/offsets.rs` | `game/offsets.hpp` | *generated* (580 consts) |
| `game/structs.rs` | `iw4structs.hpp` | `#[repr(C)]` + const size asserts |
| `game/funcs.rs` | `iw4hooks.*`, `game/functions.cpp` | engine fns via `extern "cdecl"` |
| `state.rs` | `vars` / `accent_color` | all defaults + lookup tables |
| `functions.rs` | `dismay/functions.cpp` | dvars, patches, server cmds, DLC, rank/XP, hotkeys, worker thread |
| `config.rs` | `config.cpp` | byte-compatible `dismay_config.ini` |
| `crosshair_sharecode.rs` | `crosshair_sharecode.cpp` | **round-trip unit-tested** |
| `unlock_all.rs` | `sendUnlockAllClients` | *generated* 486-cmd list |
| `win32.rs` | `<Windows.h>` | hand-FFI for the core's ~20 calls |

**Injection + hook spine** — compiles against the real D3D9 API (runtime-validated only in-game):

| Rust | C++ | Notes |
|---|---|---|
| `lib.rs` | `dllmain.cpp` | `DllMain` + bootstrap thread |
| `gui.rs` | `gui.cpp` (setup half) | D3D9 probe device → shared vtable; window subclass; INSERT toggle; `setup_menu` |
| `hooks.rs` | `hooks.hpp` | VMT hook of `EndScene` (42) / `Reset` (16); worker+config start on first setup frame |
| `assets.rs` | `fonts.h`, `fa.h`, `images.h` | the 5 byte arrays **extracted to `assets/*.ttf|gif`**, byte-identical, `include_bytes!` |

**Pure-Rust renderer** — compiles; built directly (no ImGui) so it looks like the
original. Runtime-validatable only in-game:

| Rust | C++ | Notes |
|---|---|---|
| `render/draw_list.rs` | ImGui `ImDrawList` | rects/rounded-rects/lines/circles/convex-poly/text/image → vtx+idx + clip/texture cmds |
| `render/dx9.rs` | `imgui_impl_dx9.cpp` | dynamic VB/IB, state-block save/restore, alpha blend, scissor, pre-transformed verts; font texture; `Reset` invalidate/recreate |
| `render/font.rs` | ImGui atlas + `CalcTextSizeA` | bakes Roboto/Morpheus/FA into one `ab_glyph` atlas; measure + draw, scaled per call |
| `render/input.rs` | `imgui_impl_win32` | mouse/wheel capture from the subclassed wnd-proc; per-frame snapshot |
| `render/context.rs` | ImGui frame/IO | per-frame `Ui` (draw list + fonts + input + hit-testing) |
| `render/menu.rs` | `gui.cpp::Render`, `DrawCrosshairOverlay` | window chrome, title pill + “DISMAY”, animated clickable **tab strip**, body columns; crosshair overlay (faithful) |
| hook wiring | `hooks.hpp`/`gui.cpp` | `EndScene` → build+render frame; INSERT toggles; `Reset` → invalidate |

## Remaining ⏳

| Pending | C++ source |
|---|---|
| `framework/*` widgets (Checkbox/Slider/Combo/Button/Child/Table/Text/notify) | `framework/*` |
| per-tab page content (Main/Account/Host/Dedigamer/About) — bodies are placeholders | `menu/pages/*` |
| `lain.gif` decode (`image`) + animation in the title pill | `ksd::D3D9MemoryGif_*` |
| title “666” pulse + neon glow; watermark | `gui.cpp::UpdateMenuTitle`, `watermark.cpp` |
| FA tab-icon codepoints (verify against `IconsFontAwesome6.h`) | `framework/fonts.h` |
| `dedigamer` account/network feature | `dedigamer.*` |

> ⚠️ The renderer + hooks **compile** but have **not** been run in MW2. The DX9
> state setup, vtable indices, color order, and atlas all need on-target
> validation; expect iteration on first injection.
