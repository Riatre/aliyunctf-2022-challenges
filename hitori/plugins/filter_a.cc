#include "d.h"
#include "plugin.h"
#include "plugins/gaussian_blur.h"

namespace {

using hitori::plugins::helpers::GaussianBlur;
using hitori::plugins::helpers::LibD;
using hitori::plugins::helpers::CanvasToMat;

class FilterA : public hitori::Plugin {
 public:
  ~FilterA() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Filter A"; }
  std::string Description() const override { return "This is a filter plugin."; }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    for (size_t c = 0; c < hitori::kChannels; c++) {
      GaussianBlur::GetInstance().Apply(CanvasToMat(canvas, c));
    }
    LibD::GetInstance().Use();
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static FilterA instance;
  return &instance;
}
