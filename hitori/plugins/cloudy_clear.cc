#include "plugin.h"
#include "plugins/histogram_equalization.h"
#include "plugins/median_filter.h"

namespace {

using hitori::plugins::CanvasToMat;
using hitori::plugins::HistogramEqualization;
using hitori::plugins::MedianFilter3x3;

class CloudyClear : public hitori::Plugin {
 public:
  ~CloudyClear() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Cloudy Clear"; }
  std::string Description() const override {
    return "The filter unironically makes your photo cloudy (? ? ?) clear.";
  }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    for (size_t c = 0; c < hitori::kChannels; c++) {
      auto image = CanvasToMat(canvas, c);
      MedianFilter3x3().Apply(image);
      HistogramEqualization().GlobalHistogramEqualization(image);
    }
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static CloudyClear instance;
  return &instance;
}
