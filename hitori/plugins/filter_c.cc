#include "canvas.h"
#include "f.h"
#include "plugin.h"
#include "plugins/image_utils.h"
#include "plugins/laplacian.h"

namespace {

class FilterC : public hitori::Plugin {
 public:
  ~FilterC() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Filter C"; }
  std::string Description() const override { return "This is a filter plugin."; }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    hitori::plugins::helpers::Laplacian::GetInstance().Apply(
        hitori::plugins::helpers::CanvasToMat(canvas, 0));
    auto im = hitori::plugins::helpers::CanvasToHWC(canvas);
    for (size_t c = 1; c < hitori::kChannels; c++) {
      for (size_t h = 0; h < canvas.height(); h++) {
        for (size_t w = 0; w < canvas.width(); w++) {
          im(h, w, c) = im(h, w, 0);
        }
      }
    }
    hitori::plugins::helpers::LibF::GetInstance().Use();
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static FilterC instance;
  return &instance;
}
