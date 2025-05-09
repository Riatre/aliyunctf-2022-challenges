#include "plugin.h"
#include "plugins/edge_detector.h"
#include "plugins/color_converter.h"

namespace {

using hitori::plugins::EdgeDetector;
using hitori::plugins::CanvasToHWC;
using hitori::plugins::CanvasToMat;
using hitori::plugins::ColorConverter;

class CuttingEdgePlugin : public hitori::Plugin {
 public:
  ~CuttingEdgePlugin() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Edge-o-Matic"; }
  std::string Description() const override {
    return "A magic wand for everything about edge. Works very fast! Accurate edging! 100% "
           "frustration guaranteed... oh sorry, 100% satisfaction, I mean.";
  }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    auto grayed = canvas;
    ColorConverter::GetInstance().RGBToGrayscale(CanvasToHWC(grayed));
    EdgeDetector::GetInstance().Apply(CanvasToMat(grayed, 0));
    for (size_t y = 0; y < canvas.height(); y++) {
      for (size_t x = 0; x < canvas.width(); x++) {
        auto [r, g, b] = grayed.GetPixel(x, y);
        canvas.SetPixel(x, y, {r, r, r});
      }
    }
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static CuttingEdgePlugin instance;
  return &instance;
}
