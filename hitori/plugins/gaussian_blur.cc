#include "plugins/gaussian_blur.h"

#include <cmath>
#include <cstdlib>
#include <algorithm>

namespace {

double Gaussian(double x, double mu, double sigma) {
  double a = (x - mu) / sigma;
  return exp(-0.5 * a * a);
}

}  // namespace

namespace hitori::plugins {

GaussianBlur::GaussianBlur() { InitKernel(); }

GaussianBlur::~GaussianBlur() = default;

void GaussianBlur::InitKernel() {
  for (size_t i = 0; i < 2 * kRadius + 1; ++i) {
    for (size_t j = 0; j < 2 * kRadius + 1; ++j) {
      kernel_[i][j] = 0;
    }
  }
  constexpr double sigma = kRadius / 2.;
  double sum = 0.0;
  for (size_t i = 0; i < 2 * kRadius + 1; ++i) {
    for (size_t j = 0; j < 2 * kRadius + 1; ++j) {
      kernel_[i][j] = Gaussian(i, kRadius, sigma) * Gaussian(j, kRadius, sigma);
      sum += kernel_[i][j];
    }
  }
  for (size_t i = 0; i < 2 * kRadius + 1; ++i) {
    for (size_t j = 0; j < 2 * kRadius + 1; ++j) {
      kernel_[i][j] /= sum;
    }
  }
}

void GaussianBlur::Apply(Mat image) const {
  static_assert(image.rank() == 2, "should be a CHW image with implicit C=1");
  // Conv2D with implicit padding. Gosh this is the most naive conv implementation I've ever seen.
  for (size_t i = 0; i < image.extent(0); ++i) {
    for (size_t j = 0; j < image.extent(1); ++j) {
      double sum = 0.0;
      for (size_t ki = 0; ki < 2 * kRadius + 1; ++ki) {
        for (size_t kj = 0; kj < 2 * kRadius + 1; ++kj) {
          uint8_t val = 0;
          if (size_t ii = i + ki - kRadius, ij = j + kj - kRadius;
              ii >= 0 && ii < image.extent(0) && ij >= 0 && ij < image.extent(1)) {
            val = image(ii, ij);
          }
          sum += kernel_[ki][kj] * val;
        }
      }
      image(i, j) = static_cast<uint8_t>(std::clamp(std::round(sum), 0., 255.));
    }
  }
}

}  // namespace hitori::plugins
