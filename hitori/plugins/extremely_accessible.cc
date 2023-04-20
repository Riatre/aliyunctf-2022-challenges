#include "canvas.h"
#include "plugin.h"
#include "plugins/automagical_edge_detector.h"
#include "plugins/enhancer.h"
#include "plugins/median_filter.h"

namespace {

using namespace hitori::plugins::helpers;

class ExtremelyAccessible : public hitori::Plugin {
 public:
  ~ExtremelyAccessible() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Extremely Accessible"; }
  std::string Description() const override {
    return "Have issues passing accessibility review? You are lucky today! Try our latest edge "
           "brightener which should please the reviewer even if they are dogs. a11y people "
           "definitely hate this!";
  }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    auto mask_canvas = canvas.Clone();
    auto mask = CanvasToMat(mask_canvas, 0);
    MedianFilter3x3::GetInstance().Apply(mask);
    hitori::plugins::EdgeDetector::GetInstance().Apply(mask);
    Enhancer::GetInstance().MaskedBrighten(CanvasToHWC(canvas), 1.2, mask);
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static ExtremelyAccessible instance;
  return &instance;
}
