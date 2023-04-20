#pragma once

#include <memory>

#include "plugins/image_utils.h"

namespace hitori::plugins::helpers {

class Sobel {
 public:
  Sobel();
  ~Sobel();
  
  Sobel(const Sobel&) = delete;
  Sobel& operator=(const Sobel&) = delete;
  Sobel(Sobel&&) = delete;
  Sobel& operator=(Sobel&&) = delete;

  void Apply(Mat image, double threshold = 73.31) const;
  
  static Sobel& GetInstance() {
    static Sobel inst;
    return inst;
  }

 private:
  void InitKernel();

  double gx_[3][3];
  double gy_[3][3];
};

}  // namespace hitori::plugins::helpers
