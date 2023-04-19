#pragma once

#include <memory>
#include "plugins/image_utils.h"

namespace hitori::plugins::helpers {

class MedianFilter3x3 {
 public:
  MedianFilter3x3();
  ~MedianFilter3x3();

  MedianFilter3x3(const MedianFilter3x3&) = delete;
  MedianFilter3x3& operator=(const MedianFilter3x3&) = delete;
  MedianFilter3x3(MedianFilter3x3&&) = delete;
  MedianFilter3x3& operator=(MedianFilter3x3&&) = delete;

  void Apply(Mat image);

  static MedianFilter3x3& GetInstance() {
    static MedianFilter3x3 inst;
    return inst;
  }
 private:
  // Sorting window, how sus!
  uint8_t window_[9];
};

}  // namespace hitori::plugins::helpers
