#pragma once

#include <memory>

#include "plugins/image_utils.h"

namespace hitori::plugins {

class GaussianBlur {
 public:
  
  constexpr static size_t kRadius = 5;
  GaussianBlur();
  ~GaussianBlur();
  
  GaussianBlur(const GaussianBlur&) = delete;
  GaussianBlur& operator=(const GaussianBlur&) = delete;
  GaussianBlur(GaussianBlur&&) = delete;
  GaussianBlur& operator=(GaussianBlur&&) = delete;

  void Apply(Mat image) const;

  static GaussianBlur& GetInstance() {
    static GaussianBlur inst;
    return inst;
  }

 private:
  void InitKernel();

  double kernel_[2*kRadius+1][2*kRadius+1];
};

}  // namespace hitori::plugins
