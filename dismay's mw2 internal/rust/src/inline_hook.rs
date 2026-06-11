//! Minimal pure-Rust inline (detour) hook for 32-bit x86 — our stand-in for
//! MinHook (the Rust detour crates all hardcode the `win64` ABI and won't
//! compile for i686).
//!
//! Two strategies:
//!  1. **Hot-patch** — retail system DLLs (incl. `d3d9.dll`) begin functions
//!     with `mov edi, edi` (`8B FF`) preceded by 5 bytes of padding. We write a
//!     long jump into the padding and a short jump over the `mov edi, edi`. No
//!     disassembly needed; the original is callable at `target + 2`.
//!  2. **Trampoline** — copy ≥5 whole instructions to a fresh executable buffer
//!     (relocating `E8/E9` rel32), append a jump back, and patch a 5-byte jump at
//!     the target. A conservative length decoder **fails safe** (no hook) on any
//!     opcode it doesn't recognise rather than risk a bad trampoline.

#![allow(dead_code)]

use crate::win32::*;
use core::ffi::c_void;

/// True if `addr` is committed, executable memory.
pub fn is_executable(addr: usize) -> bool {
    if addr == 0 {
        return false;
    }
    let mut mbi = MEMORY_BASIC_INFORMATION {
        BaseAddress: core::ptr::null_mut(),
        AllocationBase: core::ptr::null_mut(),
        AllocationProtect: 0,
        RegionSize: 0,
        State: 0,
        Protect: 0,
        Type: 0,
    };
    let n = unsafe {
        VirtualQuery(
            addr as *const c_void,
            &mut mbi,
            core::mem::size_of::<MEMORY_BASIC_INFORMATION>(),
        )
    };
    if n == 0 || mbi.State != MEM_COMMIT_STATE {
        return false;
    }
    matches!(
        mbi.Protect & 0xFF,
        PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY
    )
}

/// Install a detour. Returns the address to call the original through, or `None`
/// if the prologue couldn't be handled safely.
pub unsafe fn install(target: usize, detour: usize) -> Option<usize> {
    let head = core::slice::from_raw_parts(target as *const u8, 16);
    let pre = core::slice::from_raw_parts((target - 5) as *const u8, 5);
    crate::log::log(&format!(
        "inline: target=0x{target:08X} pre5={:02X?} head={:02X?}",
        pre,
        &head[..8]
    ));

    // 1) hot-patch
    if head[0] == 0x8B
        && head[1] == 0xFF
        && (pre.iter().all(|&b| b == 0xCC) || pre.iter().all(|&b| b == 0x90))
    {
        if hotpatch(target, detour) {
            crate::log::log("inline: hot-patch applied");
            return Some(target + 2);
        }
    }

    // 2) trampoline
    match build_trampoline(target, detour) {
        Some(orig) => {
            crate::log::log(&format!("inline: trampoline applied, orig=0x{orig:08X}"));
            Some(orig)
        }
        None => {
            crate::log::log("inline: FAILED — unrecognised prologue, not hooked");
            None
        }
    }
}

unsafe fn hotpatch(target: usize, detour: usize) -> bool {
    let start = target - 5;
    let mut old = 0u32;
    if VirtualProtect(start as *mut c_void, 7, PAGE_EXECUTE_READWRITE, &mut old) == 0 {
        return false;
    }
    // E9 rel32 at the padding -> detour
    *(start as *mut u8) = 0xE9;
    let rel = (detour as u32).wrapping_sub((start as u32).wrapping_add(5));
    core::ptr::write_unaligned((start + 1) as *mut u32, rel);
    // EB F9 (jmp short -7) at target -> the padding jump
    *(target as *mut u8) = 0xEB;
    *((target + 1) as *mut u8) = 0xF9;
    VirtualProtect(start as *mut c_void, 7, old, &mut old);
    FlushInstructionCache(GetCurrentProcess(), start as *const c_void, 7);
    true
}

unsafe fn build_trampoline(target: usize, detour: usize) -> Option<usize> {
    let code = core::slice::from_raw_parts(target as *const u8, 32);

    let mut copied = 0usize;
    let mut rel_fixups: RelList = RelList::new();
    while copied < 5 {
        let (len, rel) = insn_len(&code[copied..])?;
        if let Some(r) = rel {
            rel_fixups.push(copied + r);
        }
        copied += len;
        if copied > 24 {
            return None;
        }
    }

    let total = copied + 5;
    let tramp = VirtualAlloc(
        core::ptr::null_mut(),
        total,
        MEM_COMMIT | MEM_RESERVE,
        PAGE_EXECUTE_READWRITE,
    ) as *mut u8;
    if tramp.is_null() {
        return None;
    }
    core::ptr::copy_nonoverlapping(target as *const u8, tramp, copied);

    // Relocate any copied E8/E9 rel32.
    for k in 0..rel_fixups.len {
        let off = rel_fixups.data[k];
        let orig_rel = core::ptr::read_unaligned((target + off) as *const u32);
        let abs = (target as u32)
            .wrapping_add(off as u32)
            .wrapping_add(4)
            .wrapping_add(orig_rel);
        let new_rel = abs.wrapping_sub((tramp as u32).wrapping_add(off as u32).wrapping_add(4));
        core::ptr::write_unaligned(tramp.add(off) as *mut u32, new_rel);
    }

    // Jump back to target+copied.
    let jmp = tramp.add(copied);
    *jmp = 0xE9;
    let back = (target as u32)
        .wrapping_add(copied as u32)
        .wrapping_sub((jmp as u32).wrapping_add(5));
    core::ptr::write_unaligned(jmp.add(1) as *mut u32, back);

    // Patch the target with a jump to the detour + NOP fill.
    let mut old = 0u32;
    if VirtualProtect(target as *mut c_void, copied, PAGE_EXECUTE_READWRITE, &mut old) == 0 {
        return None;
    }
    *(target as *mut u8) = 0xE9;
    let rel = (detour as u32).wrapping_sub((target as u32).wrapping_add(5));
    core::ptr::write_unaligned((target + 1) as *mut u32, rel);
    for k in 5..copied {
        *((target + k) as *mut u8) = 0x90;
    }
    VirtualProtect(target as *mut c_void, copied, old, &mut old);
    FlushInstructionCache(GetCurrentProcess(), target as *const c_void, copied);

    Some(tramp as usize)
}

/// Tiny fixed-capacity list (avoids a heap alloc in the hook path).
struct RelList {
    data: [usize; 8],
    len: usize,
}
impl RelList {
    fn new() -> Self {
        RelList { data: [0; 8], len: 0 }
    }
    fn push(&mut self, v: usize) {
        if self.len < 8 {
            self.data[self.len] = v;
            self.len += 1;
        }
    }
}

/// Length of the ModRM operand (modrm + optional SIB + displacement).
fn modrm_len(code: &[u8], idx: usize) -> Option<usize> {
    let modrm = *code.get(idx)?;
    let md = modrm >> 6;
    let rm = modrm & 7;
    let mut len = 1usize;
    let mut base = rm;
    if md != 3 && rm == 4 {
        let sib = *code.get(idx + 1)?;
        len += 1;
        base = sib & 7;
    }
    match md {
        0 => {
            if rm == 5 || (rm == 4 && base == 5) {
                len += 4;
            }
        }
        1 => len += 1,
        2 => len += 4,
        _ => {}
    }
    Some(len)
}

/// Decode one instruction's length. Returns `(len, Some(rel32_offset))` for
/// `E8/E9` so the caller can relocate. `None` = unrecognised → abort.
fn insn_len(code: &[u8]) -> Option<(usize, Option<usize>)> {
    let mut i = 0usize;
    let mut opsize16 = false;
    loop {
        match *code.get(i)? {
            0x66 => {
                opsize16 = true;
                i += 1;
            }
            0xF0 | 0xF2 | 0xF3 | 0x2E | 0x36 | 0x3E | 0x26 | 0x64 | 0x65 => i += 1,
            0x67 => return None, // address-size override — bail
            _ => break,
        }
    }
    let imm_z = if opsize16 { 2 } else { 4 };
    let op = *code.get(i)?;
    i += 1;
    match op {
        0x50..=0x5F | 0x40..=0x4F => Some((i, None)), // push/pop/inc/dec r32
        0x90 | 0x98 | 0x99 | 0x9C | 0x9D | 0xCC | 0xF4 | 0xF5 | 0xF8 | 0xF9 | 0xFA | 0xFB | 0xFC
        | 0xFD | 0xC3 | 0xCB => Some((i, None)),
        0xC2 => Some((i + 2, None)), // ret imm16
        0x88 | 0x89 | 0x8A | 0x8B | 0x8D | 0x63 | 0x00 | 0x01 | 0x02 | 0x03 | 0x08 | 0x09 | 0x0A
        | 0x0B | 0x10 | 0x11 | 0x12 | 0x13 | 0x18 | 0x19 | 0x1A | 0x1B | 0x20 | 0x21 | 0x22
        | 0x23 | 0x28 | 0x29 | 0x2A | 0x2B | 0x30 | 0x31 | 0x32 | 0x33 | 0x38 | 0x39 | 0x3A
        | 0x3B | 0x84 | 0x85 | 0x86 | 0x87 => {
            let m = modrm_len(code, i)?;
            Some((i + m, None))
        }
        0x80 | 0x83 | 0x6B | 0xC0 | 0xC1 => {
            let m = modrm_len(code, i)?;
            Some((i + m + 1, None))
        }
        0x81 | 0x69 => {
            let m = modrm_len(code, i)?;
            Some((i + m + imm_z, None))
        }
        0xC6 => {
            let m = modrm_len(code, i)?;
            Some((i + m + 1, None))
        }
        0xC7 => {
            let m = modrm_len(code, i)?;
            Some((i + m + imm_z, None))
        }
        0xB0..=0xB7 => Some((i + 1, None)),
        0xB8..=0xBF => Some((i + imm_z, None)),
        0x6A => Some((i + 1, None)),
        0x68 => Some((i + imm_z, None)),
        0xA0..=0xA3 => Some((i + 4, None)),
        0xA8 | 0x04 | 0x0C | 0x14 | 0x1C | 0x24 | 0x2C | 0x34 | 0x3C => Some((i + 1, None)),
        0xA9 | 0x05 | 0x0D | 0x15 | 0x1D | 0x25 | 0x2D | 0x35 | 0x3D => Some((i + imm_z, None)),
        0xE8 | 0xE9 => Some((i + 4, Some(i))), // call/jmp rel32 — relocate
        0xEB | 0x70..=0x7F | 0xE0..=0xE3 => None, // short relative — bail
        0xFE | 0xFF => {
            let m = modrm_len(code, i)?;
            Some((i + m, None))
        }
        0xF6 => {
            let modrm = *code.get(i)?;
            let m = modrm_len(code, i)?;
            let reg = (modrm >> 3) & 7;
            Some((i + m + if reg <= 1 { 1 } else { 0 }, None))
        }
        0xF7 => {
            let modrm = *code.get(i)?;
            let m = modrm_len(code, i)?;
            let reg = (modrm >> 3) & 7;
            Some((i + m + if reg <= 1 { imm_z } else { 0 }, None))
        }
        0x0F => {
            let op2 = *code.get(i)?;
            let j = i + 1;
            match op2 {
                0x31 | 0xA2 => Some((j, None)), // rdtsc / cpuid
                0x80..=0x8F => None,            // jcc rel32 — bail
                0x10..=0x17 | 0x28..=0x2F | 0x40..=0x4F | 0x50..=0x6F | 0x90..=0x9F | 0xB6
                | 0xB7 | 0xBE | 0xBF | 0xAF | 0xD0..=0xFE => {
                    let m = modrm_len(code, j)?;
                    Some((j + m, None))
                }
                _ => None,
            }
        }
        _ => None,
    }
}
