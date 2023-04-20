#include "plugins/enhancer.h"

#include <algorithm>
#include <cstdlib>

#include "absl/log/check.h"
#include "plugins/image_utils.h"

namespace {

std::tuple<float, float, float> RGB2HSV(float r, float g, float b) {
  float h, s, v;
  auto [cmin, cmax] = std::minmax({r, g, b});
  float delta = cmax - cmin;

  if (delta > 0) {
    if (cmax == r) {
      h = 60 * (fmod(((g - b) / delta), 6));
    } else if (cmax == g) {
      h = 60 * (((b - r) / delta) + 2);
    } else if (cmax == b) {
      h = 60 * (((r - g) / delta) + 4);
    } else {
      // This should never happen.
      __builtin_trap();
    }

    if (cmax > 0) {
      s = delta / cmax;
    } else {
      s = 0;
    }

    v = cmax;
  } else {
    h = 0;
    s = 0;
    v = cmax;
  }

  if (h < 0) {
    h = 360 + h;
  }
  return {h, s, v};
}

std::tuple<float, float, float> RGB2HSV(uint8_t r, uint8_t g, uint8_t b) {
  return RGB2HSV(r / 255.0f, g / 255.0f, b / 255.0f);
}

std::tuple<uint8_t, uint8_t, uint8_t> HSV2RGB(float h, float s, float v) {
  float r, g, b;
  float chroma = v * s;
  float hh = fmod(h / 60.0, 6);
  float x = chroma * (1 - fabs(fmod(hh, 2) - 1));
  float m = v - chroma;

  switch (int(floor(hh))) {
    case 0:
      r = chroma;
      g = x;
      b = 0;
      break;
    case 1:
      r = x;
      g = chroma;
      b = 0;
      break;
    case 2:
      r = 0;
      g = chroma;
      b = x;
      break;
    case 3:
      r = 0;
      g = x;
      b = chroma;
      break;
    case 4:
      r = x;
      g = 0;
      b = chroma;
      break;
    case 5:
      r = chroma;
      g = 0;
      b = x;
      break;
    default:
      r = g = b = 0;
      break;
  }
  r += m;
  g += m;
  b += m;

  return {r * 255, g * 255, b * 255};
}

}  // namespace

namespace hitori::plugins::helpers {

Enhancer::Enhancer() {}
Enhancer::~Enhancer() = default;

void Enhancer::IncreaseSaturation(HWCMat rgb, double factor) const {
  // Convert RGB to HSV on the fly, increase saturation, convert back.
  for (size_t i = 0; i < rgb.extent(0); i++) {
    for (size_t j = 0; j < rgb.extent(1); j++) {
      auto [h, s, v] = RGB2HSV(rgb(i, j, 0), rgb(i, j, 1), rgb(i, j, 2));
      s = std::clamp(s * factor, 0.0, 1.0);
      auto [r, g, b] = HSV2RGB(h, s, v);
      rgb(i, j, 0) = r;
      rgb(i, j, 1) = g;
      rgb(i, j, 2) = b;
    }
  }
}

void Enhancer::Brighten(HWCMat image, double factor) const {
  // Convert RGB to HSV on the fly, increase brightness, convert back.
  for (size_t i = 0; i < image.extent(0); i++) {
    for (size_t j = 0; j < image.extent(1); j++) {
      auto [h, s, v] = RGB2HSV(image(i, j, 0), image(i, j, 1), image(i, j, 2));
      v = std::clamp(v * factor, 0.0, 1.0);
      auto [r, g, b] = HSV2RGB(h, s, v);
      image(i, j, 0) = r;
      image(i, j, 1) = g;
      image(i, j, 2) = b;
    }
  }
}

void Enhancer::MaskedBrighten(HWCMat image, double factor, const Mat mask) const {
  CHECK(image.extents() == mask.extents());
  for (size_t i = 0; i < image.extent(0); i++) {
    for (size_t j = 0; j < image.extent(1); j++) {
      if (mask(i, j) == 0) {
        continue;
      }
      auto [h, s, v] = RGB2HSV(image(i, j, 0), image(i, j, 1), image(i, j, 2));
      v = std::clamp(v * factor, 0.0, 1.0);
      auto [r, g, b] = HSV2RGB(h, s, v);
      image(i, j, 0) = r;
      image(i, j, 1) = g;
      image(i, j, 2) = b;
    }
  }
}

}  // namespace hitori::plugins::helpers
