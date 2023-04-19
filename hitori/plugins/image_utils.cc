#include "plugins/image_utils.h"

#include "absl/log/check.h"

namespace hitori::plugins::helpers {

Mat CanvasToMat(Canvas& canvas, size_t channel_index) {
  CHECK(channel_index < 3);
  return {canvas.data() + channel_index,
          {stdex::dextents<size_t, 2>{canvas.height(), canvas.width()},
           std::array<size_t, 2>{canvas.width() * kChannels, kChannels}}};
}

HWCMat CanvasToHWC(Canvas& canvas) {
  return HWCMat(canvas.data(), canvas.height(), canvas.width(), 3);
}

CHWMat CanvasToCHW(Canvas& canvas) {
  return {canvas.data(),
          {stdex::dextents<size_t, 3>{3, canvas.height(), canvas.width()},
           std::array<size_t, 3>{1, canvas.width() * kChannels, kChannels}}};
}

}  // namespace hitori::plugins::helpers
