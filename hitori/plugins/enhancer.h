#pragma once

#include <memory>

#include "plugins/image_utils.h"

namespace hitori::plugins {

class Enhancer {
 public:
  Enhancer();
  ~Enhancer();

  Enhancer(const Enhancer&) = delete;
  Enhancer& operator=(const Enhancer&) = delete;
  Enhancer(Enhancer&&) = delete;
  Enhancer& operator=(Enhancer&&) = delete;

  void IncreaseSaturation(HWCMat image, double factor) const;
  void Brighten(HWCMat image, double factor) const;
  void MaskedBrighten(HWCMat image, double factor, const Mat mask) const;

  static Enhancer& GetInstance() {
    static Enhancer inst;
    return inst;
  }
};

}  // namespace hitori::plugins
