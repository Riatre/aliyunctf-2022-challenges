#include "laplacian.h"

#include <cstdlib>

namespace hitori::plugins::helpers {

Laplacian::Laplacian() {
  InitKernel();
}

Laplacian::~Laplacian() = default;

void Laplacian::InitKernel() {
  for (size_t i = 0; i < 3; ++i) {
    for (size_t j = 0; j < 3; ++j) {
      kernel_[i][j] = 0;
    }
  }
  kernel_[0][1] = 1;
  kernel_[1][0] = 1;
  kernel_[1][2] = 1;
  kernel_[2][1] = 1;
  kernel_[1][1] = -4;
}

void Laplacian::Apply(Mat image, double threshold) const {
  static_assert(image.rank() == 2, "should be a grayscale image");
  for (size_t i = 0; i < image.extent(0); ++i) {
    for (size_t j = 0; j < image.extent(1); ++j) {
      double sum = 0.0;
      for (size_t ki = 0; ki < 3; ++ki) {
        for (size_t kj = 0; kj < 3; ++kj) {
          uint8_t val = 0;
          if (size_t ii = i + ki - 1, ij = j + kj - 1;
              ii >= 0 && ii < image.extent(0) && ij >= 0 && ij < image.extent(1)) {
            val = image(ii, ij);
          } else {
            sum = 0.0;
            break;
          }
          sum += kernel_[ki][kj] * val;
        }
      }
      image(i, j) = sum > threshold ? 255.0 : 0.0;
    }
  }
}

}  // namespace hitori::plugins::helpers
