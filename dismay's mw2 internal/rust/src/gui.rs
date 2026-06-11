//! Window + DirectX 9 bring-up and the menu render entry point. Port of
//! `menu/gui.cpp` (the device-acquisition + setup spine; the ImGui-equivalent
//! `Render()` is built on the pure-Rust renderer, still in progress).
//!
//! `SetupDirectX` here creates a throwaway NULLREF device purely to read the
//! `IDirect3DDevice9` vtable — whose function pointers are shared with the game's
//! real device — so `hooks` can intercept `EndScene`/`Reset`.

#![allow(non_snake_case)]

use crate::gui_state;
use crate::win32;
use core::ffi::c_void;
use core::sync::atomic::{AtomicUsize, Ordering};
use windows::core::{Interface, PCSTR};
use windows::Win32::Foundation::{HINSTANCE, HWND, LPARAM, LRESULT, WPARAM};
use windows::Win32::Graphics::Direct3D9::{
    Direct3DCreate9, IDirect3DDevice9, D3DADAPTER_DEFAULT, D3DCREATE_DISABLE_DRIVER_MANAGEMENT,
    D3DCREATE_SOFTWARE_VERTEXPROCESSING, D3DDEVICE_CREATION_PARAMETERS, D3DDEVTYPE_NULLREF,
    D3DFMT_UNKNOWN, D3DPRESENT_PARAMETERS, D3DSWAPEFFECT_DISCARD, D3D_SDK_VERSION,
};
use windows::Win32::System::LibraryLoader::GetModuleHandleA;
use windows::Win32::UI::WindowsAndMessaging::{
    CallWindowProcA, CreateWindowExA, DefWindowProcA, DestroyWindow, GetForegroundWindow,
    RegisterClassExA, SetWindowLongPtrA, UnregisterClassA, CS_HREDRAW, CS_VREDRAW, GWLP_WNDPROC,
    WINDOW_EX_STYLE, WNDCLASSEXA, WNDCLASS_STYLES, WNDPROC, WS_OVERLAPPEDWINDOW,
};

const WINDOW_CLASS: PCSTR = PCSTR(b"hackClass001\0".as_ptr());
const WINDOW_NAME: PCSTR = PCSTR(b"Hack Window\0".as_ptr());

/// Base address of the shared `IDirect3DDevice9` vtable (set by [`acquire_vtable`]).
static VTABLE_BASE: AtomicUsize = AtomicUsize::new(0);
/// Address of `IDirect3DDevice9::EndScene` (vtable[42]) — what `hooks` inline-hooks.
static ENDSCENE_ADDR: AtomicUsize = AtomicUsize::new(0);
/// Address of `IDirect3DDevice9::Reset` (vtable[16]).
static RESET_ADDR: AtomicUsize = AtomicUsize::new(0);
/// The game window we subclass (set in [`setup_menu`]).
static WINDOW: AtomicUsize = AtomicUsize::new(0);
/// Original window procedure, saved when subclassing.
static ORIGINAL_WNDPROC: AtomicUsize = AtomicUsize::new(0);

/// VK_INSERT — toggles the menu (`WindowProcess` in the C++).
const VK_INSERT: u32 = 0x2D;
const WM_KEYDOWN: u32 = 0x0100;

#[inline]
pub fn vtable_base() -> usize {
    VTABLE_BASE.load(Ordering::Acquire)
}
#[inline]
pub fn endscene_addr() -> usize {
    ENDSCENE_ADDR.load(Ordering::Acquire)
}
#[inline]
pub fn reset_addr() -> usize {
    RESET_ADDR.load(Ordering::Acquire)
}

/// Log which module owns `addr` (reveals a d3d9 wrapper/proxy if present).
fn log_module(label: &str, addr: usize) {
    let mut hmod: win32::HMODULE = core::ptr::null_mut();
    let ok = unsafe {
        win32::GetModuleHandleExA(
            win32::GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS
                | win32::GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            addr as *const u8,
            &mut hmod,
        )
    };
    if ok == 0 {
        crate::log::log(&format!("{label} 0x{addr:08X}: module=<unknown>"));
        return;
    }
    let mut buf = [0u8; 260];
    let n = unsafe { win32::GetModuleFileNameA(hmod, buf.as_mut_ptr(), buf.len() as u32) } as usize;
    let name = String::from_utf8_lossy(&buf[..n.min(260)]);
    crate::log::log(&format!("{label} 0x{addr:08X}: module={name}"));
}
#[inline]
pub fn window() -> HWND {
    HWND(WINDOW.load(Ordering::Acquire) as *mut c_void)
}

/// Default proc for the throwaway probe window class.
unsafe extern "system" fn def_window_proc(
    hwnd: HWND,
    msg: u32,
    wparam: WPARAM,
    lparam: LPARAM,
) -> LRESULT {
    DefWindowProcA(hwnd, msg, wparam, lparam)
}

/// `WaitForD3D9` — block until `d3d9.dll` is loaded in the process.
fn wait_for_d3d9(max_attempts: i32) -> bool {
    for _ in 0..max_attempts {
        if unsafe { GetModuleHandleA(PCSTR(b"d3d9.dll\0".as_ptr())) }.is_ok() {
            return true;
        }
        unsafe { win32::Sleep(100) };
    }
    false
}

/// `gui::Setup` + `SetupDirectX`: spin up a probe device and record the shared
/// device vtable base. Returns true on success.
pub fn acquire_vtable() -> bool {
    const MAX_ATTEMPTS: i32 = 50;

    if !wait_for_d3d9(MAX_ATTEMPTS) {
        crate::log::log("acquire_vtable: d3d9.dll wait TIMEOUT");
        return false;
    }
    crate::log::log("acquire_vtable: d3d9.dll present");

    let hinstance = match unsafe { GetModuleHandleA(PCSTR::null()) } {
        Ok(h) => HINSTANCE(h.0),
        Err(_) => return false,
    };

    // Register a throwaway window class + hidden window.
    let wc = WNDCLASSEXA {
        cbSize: core::mem::size_of::<WNDCLASSEXA>() as u32,
        style: WNDCLASS_STYLES(CS_HREDRAW.0 | CS_VREDRAW.0),
        lpfnWndProc: Some(def_window_proc),
        hInstance: hinstance,
        lpszClassName: WINDOW_CLASS,
        ..Default::default()
    };
    if unsafe { RegisterClassExA(&wc) } == 0 {
        return false;
    }

    let hwnd = match unsafe {
        CreateWindowExA(
            WINDOW_EX_STYLE(0),
            WINDOW_CLASS,
            WINDOW_NAME,
            WS_OVERLAPPEDWINDOW,
            0,
            0,
            100,
            100,
            None,
            None,
            Some(hinstance),
            None,
        )
    } {
        Ok(h) => h,
        Err(_) => {
            unsafe { let _ = UnregisterClassA(WINDOW_CLASS, Some(hinstance)); };
            return false;
        }
    };

    let mut ok = false;
    for _ in 0..MAX_ATTEMPTS {
        if try_capture_vtable(hwnd) {
            ok = true;
            break;
        }
        unsafe { win32::Sleep(100) };
    }

    // FinalizeBootstrapSetup: tear down the probe window/class.
    unsafe {
        let _ = DestroyWindow(hwnd);
        let _ = UnregisterClassA(WINDOW_CLASS, Some(hinstance));
    }

    crate::log::log(&format!(
        "acquire_vtable: ok={ok} endscene=0x{:08X} reset=0x{:08X}",
        ENDSCENE_ADDR.load(Ordering::Acquire),
        RESET_ADDR.load(Ordering::Acquire)
    ));
    ok
}

fn try_capture_vtable(hwnd: HWND) -> bool {
    unsafe {
        let d3d9 = match Direct3DCreate9(D3D_SDK_VERSION) {
            Some(d) => d,
            None => return false,
        };

        let mut params = D3DPRESENT_PARAMETERS {
            Windowed: true.into(),
            SwapEffect: D3DSWAPEFFECT_DISCARD,
            hDeviceWindow: hwnd,
            BackBufferFormat: D3DFMT_UNKNOWN,
            ..Default::default()
        };

        let mut device = None;
        let hr = d3d9.CreateDevice(
            D3DADAPTER_DEFAULT,
            D3DDEVTYPE_NULLREF,
            hwnd,
            (D3DCREATE_SOFTWARE_VERTEXPROCESSING | D3DCREATE_DISABLE_DRIVER_MANAGEMENT) as u32,
            &mut params,
            &mut device,
        );
        if let Err(e) = hr {
            crate::log::log(&format!("try_capture_vtable: CreateDevice failed hr=0x{:08X}", e.code().0));
            return false;
        }
        let device = match device {
            Some(d) => d,
            None => return false,
        };

        // The COM object's first member is its vtable pointer.
        let obj = Interface::as_raw(&device) as *const *const c_void;
        let vtable = *obj as usize;
        if vtable == 0 {
            return false;
        }
        // EndScene = slot 42, Reset = slot 16. Validate both are real code (the
        // probe read can occasionally return garbage); retry otherwise.
        let es = *((vtable + 42 * 4) as *const usize);
        let rs = *((vtable + 16 * 4) as *const usize);
        if !crate::inline_hook::is_executable(es) || !crate::inline_hook::is_executable(rs) {
            crate::log::log(&format!(
                "try_capture_vtable: invalid slots es=0x{es:08X} rs=0x{rs:08X} (retry)"
            ));
            return false;
        }
        VTABLE_BASE.store(vtable, Ordering::Release);
        ENDSCENE_ADDR.store(es, Ordering::Release);
        RESET_ADDR.store(rs, Ordering::Release);
        log_module("EndScene", es);
        log_module("Reset", rs);
        // `device` Releases here; the code we hook lives in d3d9.dll and persists.
        true
    }
}

/// `gui::SetupMenu` (spine): subclass the game window and mark setup done so the
/// worker + config kick in. ImGui-context/font setup belongs to the renderer.
pub fn setup_menu(device_ptr: *mut c_void) {
    if gui_state::setup() {
        return;
    }

    // TODO(renderer): derive the focus window from the device creation params;
    // GetForegroundWindow is the C++ fallback path and is good enough to subclass.
    // Prefer the game's actual window from the device creation params (matches
    // the C++); GetForegroundWindow is the fallback.
    let raw = device_ptr;
    let mut hwnd = HWND(core::ptr::null_mut());
    if let Some(dev) = unsafe { IDirect3DDevice9::from_raw_borrowed(&raw) } {
        let mut cp = D3DDEVICE_CREATION_PARAMETERS::default();
        if unsafe { dev.GetCreationParameters(&mut cp) }.is_ok() {
            hwnd = cp.hFocusWindow;
        }
    }
    if hwnd.0.is_null() {
        hwnd = unsafe { GetForegroundWindow() };
    }
    if hwnd.0.is_null() {
        crate::log::log("setup_menu: no window found");
        return;
    }
    WINDOW.store(hwnd.0 as usize, Ordering::Release);

    // On i686 `SetWindowLongPtrA` is `SetWindowLongA` (LONG = i32).
    let prev = unsafe { SetWindowLongPtrA(hwnd, GWLP_WNDPROC, window_process as *const () as i32) };
    ORIGINAL_WNDPROC.store(prev as u32 as usize, Ordering::Release);

    gui_state::set_setup(true);
    crate::log::log(&format!(
        "setup_menu: subclassed hwnd=0x{:08X} prev=0x{:08X}",
        hwnd.0 as usize, prev as u32 as usize
    ));
    crate::config::Load();
}

/// `WindowProcess` — INSERT toggles the menu; everything else forwards to the
/// original proc. (ImGui input routing is added with the renderer.)
unsafe extern "system" fn window_process(
    hwnd: HWND,
    msg: u32,
    wparam: WPARAM,
    lparam: LPARAM,
) -> LRESULT {
    if msg == WM_KEYDOWN && wparam.0 as u32 == VK_INSERT {
        gui_state::toggle_open();
    }

    if gui_state::open() {
        let _ = crate::render::input::feed(msg, wparam.0, lparam.0);
        // Swallow mouse (0x0200..=0x020E) + keyboard (0x0100..=0x0108) from the
        // game while the menu is open.
        let is_mouse = (0x0200..=0x020E).contains(&msg);
        let is_key = (0x0100..=0x0108).contains(&msg);
        if is_mouse || is_key {
            return LRESULT(0);
        }
    }

    let original = ORIGINAL_WNDPROC.load(Ordering::Acquire);
    if original != 0 {
        let proc: WNDPROC = Some(core::mem::transmute::<usize, _>(original));
        return CallWindowProcA(proc, hwnd, msg, wparam, lparam);
    }
    DefWindowProcA(hwnd, msg, wparam, lparam)
}

/// `gui::Render` — draws the menu via the pure-Rust renderer.
pub fn render(device: *mut c_void) {
    crate::render::render_frame(device);
}
