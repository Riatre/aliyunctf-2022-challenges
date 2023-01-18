#include <stddef.h>
#include <array>

namespace {

char _dummy;

template<size_t N>
constexpr std::array<void*, N> GenerateDummyArrayForFillingRelocations() {
  std::array<void*, N> result{};
  for (size_t i = 0; i < N; i++) {
    result[i] = &_dummy;
  }
  return result;
}

}

// This also makes .bss larger :(
auto __reloc_filler = GenerateDummyArrayForFillingRelocations<100>();
