#include "median_filter.h"
#include "e.h"
#include "plugin.h"

namespace {

class FilterB : public hitori::Plugin {
 public:
  ~FilterB() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Filter B"; }
  std::string Description() const override { return "This is a filter plugin."; }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    for (size_t c = 0; c < hitori::kChannels; c++) {
      hitori::plugins::helpers::MedianFilter3x3::GetInstance().Apply(
          hitori::plugins::helpers::CanvasToMat(canvas, c));
    }
    hitori::plugins::helpers::Sobel::GetInstance().Use();
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static FilterB instance;
  return &instance;
}
