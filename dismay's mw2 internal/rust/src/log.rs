//! Dead-simple file logger for debugging injection. Writes to
//! `%TEMP%\dismay_rust.log`. Temporary scaffolding while bringing the hook +
//! renderer up on-target.

use std::fs::OpenOptions;
use std::io::Write;

pub fn path() -> std::path::PathBuf {
    std::env::temp_dir().join("dismay_rust.log")
}

pub fn log(msg: &str) {
    let t = unsafe { crate::win32::GetTickCount64() };
    let line = format!("[{t}ms] {msg}\n");
    if let Ok(mut f) = OpenOptions::new().create(true).append(true).open(path()) {
        let _ = f.write_all(line.as_bytes());
    }
}

/// Log Rust panics (which would otherwise just `abort` the game silently).
pub fn init_panic_hook() {
    std::panic::set_hook(Box::new(|info| {
        log(&format!("PANIC: {info}"));
    }));
}
