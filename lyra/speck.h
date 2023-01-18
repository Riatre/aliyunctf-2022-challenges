#pragma once

#include <stddef.h>
#include <stdint.h>

namespace lyra::cipher {

inline constexpr const size_t kKeySize = 16;

void ExpandKey(uint64_t subkeys[], uint64_t rawkey[2]);
void Encrypt(void *dst, const void *src, size_t size, uint64_t rawkey[2]);
void Decrypt(void *dst, const void *src, size_t size, uint64_t rawkey[2]);

} // namespace lyra::cipher
