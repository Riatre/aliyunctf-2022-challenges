#include "speck.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <string_view>

constexpr const std::string_view kPasswordPrefix = "password{";
constexpr const std::string_view kPasswordSuffix = "}";
constexpr const char *kFlagFileName = "flag.txt";

// This part is meant to be impossible! The cipher we used should not be
// distingushiable anyway (at least, at 2023). To make sure it is actually
// impossible (and I didn't insert a backdoor here for easier solution), the
// input is generated with the following one-liner (with Python 3.10.9):
// python3 -c 'import sys, random; random.seed(1145141919810);
// sys.stdout.buffer.write(random.randbytes(32))' | xxd -i` and the expected
// output is generated with 19260817 as seed.
//
// We could use all printable characters for both the input and the expected
// output if we want to tip the players this is actually impossible.
constexpr const unsigned char kInput[32] = {
    0xe9, 0xc7, 0x2c, 0x95, 0x4a, 0x21, 0x3d, 0x0c, 0x4b, 0xa1, 0x91,
    0x37, 0x19, 0xde, 0xca, 0xd7, 0x61, 0xcf, 0x28, 0x44, 0xa2, 0xc2,
    0xc8, 0xcf, 0xd8, 0xa8, 0x43, 0xd1, 0x0e, 0xef, 0x3f, 0xa4};
constexpr const unsigned char kExpected[32] = {
    0x22, 0x44, 0x59, 0xc7, 0xb7, 0x07, 0xe1, 0xd0, 0x64, 0xfe, 0xbc,
    0xc3, 0x03, 0x0b, 0xfd, 0xd5, 0xb9, 0xe6, 0x6b, 0x80, 0x8f, 0xb4,
    0x1c, 0x5e, 0x07, 0x6c, 0x8e, 0x80, 0xd2, 0x8a, 0xc4, 0x54};
static_assert(sizeof(kInput) == sizeof(kExpected));

char g_password[256];

std::ifstream OpenFlag() {
  if (!std::filesystem::exists(kFlagFileName)) {
    std::cerr
        << "Flag file not found in current directory, challenge is broken."
        << std::endl;
    abort();
  }
  return std::ifstream{kFlagFileName};
}

void AlarmHandler(int) {
  puts("Timed out.");
  exit(0);
}

bool Verify(std::string_view flag) {
  if (!flag.starts_with(kPasswordPrefix) || !flag.ends_with(kPasswordSuffix)) {
    return false;
  }
  auto key =
      flag.substr(kPasswordPrefix.length(),
                  flag.length() - kPasswordPrefix.length() - kPasswordSuffix.length());
  if (key.size() != lyla::cipher::kKeySize) {
    return false;
  }
  unsigned char output[32];
  uint64_t rawkey[2];
  static_assert(sizeof(rawkey) == lyla::cipher::kKeySize);
  memcpy(rawkey, key.data(), sizeof(rawkey));
  static_assert(sizeof(kInput) == sizeof(output));
  lyla::cipher::Encrypt(output, kInput, sizeof(output), rawkey);
  return !memcmp(output, kExpected, sizeof(kExpected));
}

int main(int argc, char *argv[]) {
  setvbuf(stdin, nullptr, _IONBF, 0);
  setvbuf(stdout, nullptr, _IONBF, 0);
  std::ios::sync_with_stdio(false);

  std::ifstream flag_file = OpenFlag();
  signal(SIGALRM, AlarmHandler);
  alarm(60);

  std::cout << "Welcome to lyla, the devious flag vending machine." << '\n'
            << "Ready to deliver orgas...\x08\x08\x08\x08nic flag tea to you!" << '\n';
  std::cout << "Input password: ";
  std::cout.flush();
  std::cin.getline(g_password, sizeof(g_password));
  if (Verify(g_password)) {
    std::cout << "Correct password! Congratulations, here is the flag:" << '\n';
    std::cout << flag_file.rdbuf();
    std::cout.flush();
  } else {
    std::cout << "Nope." << std::endl;
  }
  return 0;
}
