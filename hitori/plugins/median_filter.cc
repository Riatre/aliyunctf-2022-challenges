#include "median_filter.h"

#include <cstdlib>

namespace hitori::plugins {

MedianFilter3x3::MedianFilter3x3() {}

MedianFilter3x3::~MedianFilter3x3() { }

void MedianFilter3x3::Apply(Mat image) {
  // Apply median filter on the image.
  if (image.extent(0) < 3 || image.extent(1) < 3) {
    return;
  }
  for (size_t i = 1; i < image.extent(0) - 1; i++) {
    for (size_t j = 1; j < image.extent(1) - 1; j++) {
      window_[0] = image(i - 1, j - 1);
      window_[1] = image(i - 1, j);
      window_[2] = image(i - 1, j + 1);
      window_[3] = image(i, j - 1);
      window_[4] = image(i, j);
      window_[5] = image(i, j + 1);
      window_[6] = image(i + 1, j - 1);
      window_[7] = image(i + 1, j);
      window_[8] = image(i + 1, j + 1);
      std::sort(window_, window_ + 9);
      image(i, j) = window_[4];
    }
  }
}

}  // namespace hitori::plugins
