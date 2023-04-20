#pragma once

#include <memory>

#include "plugins/image_utils.h"

namespace hitori::plugins::helpers {

class ColorConverter {
 public:
  ColorConverter();
  ~ColorConverter();

  ColorConverter(const ColorConverter&) = delete;
  ColorConverter& operator=(const ColorConverter&) = delete;
  ColorConverter(ColorConverter&&) = delete;
  ColorConverter& operator=(ColorConverter&&) = delete;

  void RGBToGrayscale(HWCMat rgb) const;

  static ColorConverter& GetInstance() {
    static ColorConverter inst;
    return inst;
  }
};

}  // namespace hitori::plugins::helpers
