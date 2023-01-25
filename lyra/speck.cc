#include "speck.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#undef assert
#define assert(expr)                                                           \
  (static_cast<bool>(expr)                                                     \
       ? void(0)                                                               \
       : __assert_fail(#expr, "<redacted>", __LINE__, "<redacted>"));

constexpr const size_t kBlockSize = 16;
constexpr const int kRounds = 32;

namespace {

uint64_t ROR64(uint64_t x, int r) { return ((x >> r) | (x << (64 - r))); }
uint64_t ROL64(uint64_t x, int r) { return ((x << r) | (x >> (64 - r))); }

void SpeckRound(uint64_t &x, uint64_t &y, uint64_t k) {
  x = ROR64(x, 8);
  x += y;
  x ^= k;
  y = ROL64(y, 3);
  y ^= x;
}

void SpeckInverseRound(uint64_t &x, uint64_t &y, uint64_t k) {
  y ^= x;
  y = ROR64(y, 3);
  x ^= k;
  x -= y;
  x = ROL64(x, 8);
}

} // namespace

namespace lyra::cipher {

void ExpandKey(uint64_t subkeys[], uint64_t rawkey[2]) {
  uint64_t A = rawkey[0], B = rawkey[1];
  #pragma GCC unroll 32
  for (int i = 0; i < kRounds; i++) {
    subkeys[i] = A;
    SpeckRound(B, A, i);
  }
}

// TODO(riatre): Use a AVX-2 implementation here? (that would be evil but :p)
void Encrypt(void *dst, const void *src, size_t size, uint64_t rawkey[2]) {
  assert(size % 16 == 0);
  dst = __builtin_assume_aligned(dst, 16);
  src = __builtin_assume_aligned(src, 16);

  uint64_t key[kRounds];
  ExpandKey(key, rawkey);
  for (size_t i = 0; i < size; i += 16) {
    uint64_t *current_dst = reinterpret_cast<uint64_t *>(dst);
    const uint64_t *current_src = reinterpret_cast<const uint64_t *>(src);
    uint64_t x = current_src[0], y = current_src[1];
    #pragma GCC unroll 8
    for (int i = 0; i < kRounds; i++) {
      SpeckRound(y, x, key[i]);
    }
    current_dst[0] = x;
    current_dst[1] = y;
  }
}

void Decrypt(void *dst, const void *src, size_t size, uint64_t rawkey[2]) {
  assert(size % 16 == 0);
  dst = __builtin_assume_aligned(dst, 16);
  src = __builtin_assume_aligned(src, 16);

  uint64_t key[kRounds];
  ExpandKey(key, rawkey);
  for (size_t i = 0; i < size; i += 16) {
    uint64_t *current_dst = reinterpret_cast<uint64_t *>(dst);
    const uint64_t *current_src = reinterpret_cast<const uint64_t *>(src);
    uint64_t x = current_src[0], y = current_src[1];
    #pragma GCC unroll 8
    for (int i = kRounds - 1; i >= 0; i--) {
      SpeckInverseRound(y, x, key[i]);
    }
    current_dst[0] = x;
    current_dst[1] = y;
  }
}
} // namespace lyra::cipher
