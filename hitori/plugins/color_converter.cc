#include "plugins/color_converter.h"

#include <cstdlib>

#include "plugins/image_utils.h"

namespace hitori::plugins::helpers {

ColorConverter::ColorConverter() {}

ColorConverter::~ColorConverter() {}

void ColorConverter::RGBToGrayscale(HWCMat rgb) const {
  static_assert(rgb.rank() == 3);
  for (size_t y = 0; y < rgb.extent(0); ++y) {
    for (size_t x = 0; x < rgb.extent(1); ++x) {
      double r = rgb(y, x, 0);
      double g = rgb(y, x, 1);
      double b = rgb(y, x, 2);
      double gray = 0.299 * r + 0.587 * g + 0.114 * b;
      rgb(y, x, 0) = gray;
      rgb(y, x, 1) = gray;
      rgb(y, x, 2) = gray;
    }
  }
}

}  // namespace hitori::plugins::helpers
