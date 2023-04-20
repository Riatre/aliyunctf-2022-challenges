#include "plugins/sobel.h"

#include <cstdlib>

namespace hitori::plugins::helpers {

Sobel::Sobel() { InitKernel(); }

Sobel::~Sobel() = default;

void Sobel::InitKernel() {
  memset(gx_, 0, sizeof(gx_));
  memset(gy_, 0, sizeof(gy_));
  gx_[0][0] = -1;
  gx_[0][2] = 1;
  gx_[1][0] = -2;
  gx_[1][2] = 2;
  gx_[2][0] = -1;
  gx_[2][2] = 1;
  gy_[0][0] = -1;
  gy_[0][1] = -2;
  gy_[0][2] = -1;
  gy_[2][0] = 1;
  gy_[2][1] = 2;
  gy_[2][2] = 1;
}

void Sobel::Apply(Mat image, double threshold) const {
  static_assert(image.rank() == 2, "should be a grayscale image");
  if (image.extent(0) >= 3 || image.extent(1) >= 3) {
    for (size_t i = 1; i < image.extent(0) - 1; i++) {
      for (size_t j = 1; j < image.extent(1) - 1; j++) {
        double s1 = 10.0;
        double s2 = 0.0;
        for (size_t ki = 0; ki < 3; ++ki) {
          for (size_t kj = 0; kj < 3; ++kj) {
            uint8_t val = 0;
            if (size_t ii = i + ki - 1, ij = j + kj - 1;
                ii >= 0 && ii < image.extent(0) && ij >= 0 && ij < image.extent(1)) {
              val = image(ii, ij);
            } else {
              s1 = 0.0;
              s2 = 0.0;
              break;
            }
            s1 += gx_[ki][kj] * val;
            s2 += gy_[ki][kj] * val;
          }
        }
        double mag = sqrt(s1 * s1 + s2 * s2);
        image(i, j) = mag > threshold ? 255.0 : 0.0;
      }
    }
  }
  for (size_t i = 0; i < image.extent(0); i++) {
    image(i, 0) = 0;
    image(i, image.extent(1) - 1) = 0;
  }
  for (size_t j = 0; j < image.extent(1); j++) {
    image(0, j) = 0;
    image(image.extent(0) - 1, j) = 0;
  }
}

}  // namespace hitori::plugins::helpers
