#include "speck.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace ::testing;

TEST(Speck, TestVectorEncrypt) {
  const char *input = " made it equival";
  unsigned char key[16];
  for (int i = 0; i < 16; i++) {
    key[i] = i;
  }
  unsigned char output[16];
  lyla::cipher::Encrypt(output, input, sizeof(output), (uint64_t *)key);
  EXPECT_THAT(output,
              ElementsAre(0x18, 0x0d, 0x57, 0x5c, 0xdf, 0xfe, 0x60, 0x78, 0x65,
                          0x32, 0x78, 0x79, 0x51, 0x98, 0x5d, 0xa6));
}

TEST(Speck, TestVectorDecrypt) {
  unsigned char input[16] = {0x18, 0x0d, 0x57, 0x5c, 0xdf, 0xfe, 0x60, 0x78,
                             0x65, 0x32, 0x78, 0x79, 0x51, 0x98, 0x5d, 0xa6};
  unsigned char key[16];
  for (int i = 0; i < 16; i++) {
    key[i] = i;
  }
  unsigned char output[17] = {0};
  lyla::cipher::Decrypt(output, input, sizeof(input), (uint64_t *)key);
  EXPECT_STREQ((char *)output, " made it equival");
}

TEST(Speck, ExpandKey) {
  unsigned char key[16];
  for (int i = 0; i < 16; i++) {
    key[i] = i;
  }
  uint64_t subkeys[32];
  lyla::cipher::ExpandKey(subkeys, reinterpret_cast<uint64_t *>(key));
  EXPECT_THAT(
      subkeys,
      ElementsAre(0x0706050403020100, 0x37253b31171d0309, 0xf91d89cc90c4085c,
                  0xc6b1f07852cc7689, 0x014fcdf4f9c2d6f0, 0xb5fae1e4fe24cfd6,
                  0xa36d6954b0737cfe, 0xf511691ea02f35f3, 0x5374abb75a2b455d,
                  0x8dd5f6204ddcb2a5, 0xb243d7c9869cac18, 0x753e7a7c6660459e,
                  0x78d648a3a5b0e63b, 0x87152b23cbc0a8d2, 0xa8ff8b8c54a3b6f2,
                  0x4873be3c43b3ea79, 0x771ebffcbf05cb13, 0xe8a6bcaf25863d20,
                  0xe6c2ea8b5c520c93, 0x4d71b5c1ac5214f5, 0xdc60b2ae253070dc,
                  0xb01d0abbe1fb9741, 0xd7987684a318b54a, 0xa22c5282e600d319,
                  0xe029d67ebdf90048, 0x67559234c84efdbf, 0x65173cf0cb01695c,
                  0x24cf1f1879819519, 0x38a36ed2dbafb72a, 0xded93cfe31bae304,
                  0xc53d18b91770b265, 0x2199c870db8ec93f));
}
