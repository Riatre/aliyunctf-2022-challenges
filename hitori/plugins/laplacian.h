#pragma once

#include <memory>

#include "plugins/image_utils.h"

namespace hitori::plugins::helpers {

class Laplacian {
 public:
  static constexpr double kThreshold = 35.4274;
  Laplacian();
  ~Laplacian();

  Laplacian(const Laplacian&) = delete;
  Laplacian& operator=(const Laplacian&) = delete;
  Laplacian(Laplacian&&) = delete;
  Laplacian& operator=(Laplacian&&) = delete;

  void Apply(Mat image) const;

  static Laplacian& GetInstance() {
    static Laplacian inst;
    return inst;
  }

 private:
  void InitKernel();

  double kernel_[3][3];
};

}  // namespace hitori::plugins::helpers
