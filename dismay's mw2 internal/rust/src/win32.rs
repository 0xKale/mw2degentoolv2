//! Hand-written Win32 / CRT FFI declarations.
//!
//! The original C++ pulls these from `<Windows.h>`. Declaring exactly the few
//! we use (instead of taking a `windows-sys` dependency) keeps the core build
//! hermetic and lets us pin every signature ourselves. On i686 the Win32 API is
//! `stdcall`, which Rust spells `extern "system"`.

#![allow(non_snake_case, non_camel_case_types, dead_code)]

use core::ffi::c_void;

pub type BOOL = i32;
pub type DWORD = u32;
pub type HMODULE = *mut c_void;
pub type HANDLE = *mut c_void;
pub type HWND = *mut c_void;

// VirtualProtect / page protection
pub const PAGE_EXECUTE_READWRITE: DWORD = 0x40;
pub const PAGE_EXECUTE: DWORD = 0x10;
pub const PAGE_EXECUTE_READ: DWORD = 0x20;
pub const PAGE_EXECUTE_WRITECOPY: DWORD = 0x80;

// VirtualAlloc
pub const MEM_COMMIT: DWORD = 0x1000;
pub const MEM_RESERVE: DWORD = 0x2000;
pub const MEM_COMMIT_STATE: DWORD = 0x1000;

/// `MEMORY_BASIC_INFORMATION` (32-bit layout).
#[repr(C)]
#[derive(Clone, Copy)]
pub struct MEMORY_BASIC_INFORMATION {
    pub BaseAddress: *mut c_void,
    pub AllocationBase: *mut c_void,
    pub AllocationProtect: DWORD,
    pub RegionSize: usize,
    pub State: DWORD,
    pub Protect: DWORD,
    pub Type: DWORD,
}
impl Default for MEMORY_BASIC_INFORMATION {
    fn default() -> Self {
        unsafe { core::mem::zeroed() }
    }
}

// DllMain reasons
pub const DLL_PROCESS_ATTACH: DWORD = 1;

// GetModuleHandleEx flags
pub const GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS: DWORD = 0x00000004;
pub const GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT: DWORD = 0x00000002;

// File attributes
pub const INVALID_FILE_ATTRIBUTES: DWORD = 0xFFFF_FFFF;

// MessageBox flags
pub const MB_OK: DWORD = 0x0000_0000;
pub const MB_ICONEXCLAMATION: DWORD = 0x0000_0030;
pub const MB_ICONERROR: DWORD = 0x0000_0010;

// Virtual key codes used by the feature worker / window proc.
pub const VK_INSERT: i32 = 0x2D;
pub const VK_F2: i32 = 0x71;
pub const VK_F3: i32 = 0x72;
pub const VK_F4: i32 = 0x73;
pub const VK_F5: i32 = 0x74;
pub const VK_X: i32 = 0x58; // 'X'

/// `LPTHREAD_START_ROUTINE`
pub type ThreadStartRoutine = unsafe extern "system" fn(lpParameter: *mut c_void) -> DWORD;

#[link(name = "kernel32")]
extern "system" {
    pub fn VirtualProtect(
        lpAddress: *mut c_void,
        dwSize: usize,
        flNewProtect: DWORD,
        lpflOldProtect: *mut DWORD,
    ) -> BOOL;
    pub fn VirtualAlloc(
        lpAddress: *mut c_void,
        dwSize: usize,
        flAllocationType: DWORD,
        flProtect: DWORD,
    ) -> *mut c_void;
    pub fn VirtualQuery(
        lpAddress: *const c_void,
        lpBuffer: *mut MEMORY_BASIC_INFORMATION,
        dwLength: usize,
    ) -> usize;
    pub fn FlushInstructionCache(
        hProcess: HANDLE,
        lpBaseAddress: *const c_void,
        dwSize: usize,
    ) -> BOOL;
    pub fn GetCurrentProcess() -> HANDLE;

    pub fn GetTickCount64() -> u64;
    pub fn Sleep(dwMilliseconds: DWORD);

    pub fn GetModuleHandleA(lpModuleName: *const u8) -> HMODULE;
    pub fn GetProcAddress(hModule: HMODULE, lpProcName: *const u8) -> *mut c_void;
    pub fn GetModuleFileNameA(hModule: HMODULE, lpFilename: *mut u8, nSize: DWORD) -> DWORD;
    pub fn GetModuleHandleExA(
        dwFlags: DWORD,
        lpModuleName: *const u8,
        phModule: *mut HMODULE,
    ) -> BOOL;
    pub fn GetFileAttributesA(lpFileName: *const u8) -> DWORD;

    pub fn DisableThreadLibraryCalls(hLibModule: HMODULE) -> BOOL;
    pub fn CreateThread(
        lpThreadAttributes: *mut c_void,
        dwStackSize: usize,
        lpStartAddress: Option<ThreadStartRoutine>,
        lpParameter: *mut c_void,
        dwCreationFlags: DWORD,
        lpThreadId: *mut DWORD,
    ) -> HANDLE;
    pub fn CloseHandle(hObject: HANDLE) -> BOOL;

    pub fn WritePrivateProfileStringA(
        lpAppName: *const u8,
        lpKeyName: *const u8,
        lpString: *const u8,
        lpFileName: *const u8,
    ) -> BOOL;
    pub fn GetPrivateProfileIntA(
        lpAppName: *const u8,
        lpKeyName: *const u8,
        nDefault: i32,
        lpFileName: *const u8,
    ) -> u32;
    pub fn GetPrivateProfileStringA(
        lpAppName: *const u8,
        lpKeyName: *const u8,
        lpDefault: *const u8,
        lpReturnedString: *mut u8,
        nSize: DWORD,
        lpFileName: *const u8,
    ) -> DWORD;
}

#[link(name = "user32")]
extern "system" {
    pub fn GetAsyncKeyState(vKey: i32) -> i16;
    pub fn MessageBoxA(hWnd: HWND, lpText: *const u8, lpCaption: *const u8, uType: DWORD) -> i32;
    pub fn MessageBeep(uType: DWORD) -> BOOL;
    pub fn GetForegroundWindow() -> HWND;
}
