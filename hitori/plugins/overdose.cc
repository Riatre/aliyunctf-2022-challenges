#include "canvas.h"
#include "plugin.h"
#include "plugins/gaussian_blur.h"
#include "plugins/image_utils.h"

namespace {

using namespace hitori::plugins;

class Overdose : public hitori::Plugin {
 public:
  ~Overdose() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Overdose"; }
  std::string Description() const override {
    return "Exactly what you'd see when you overdose, or over mas[bi---].";
  }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    for (size_t c = 0; c < hitori::kChannels; c++) {
      GaussianBlur::GetInstance().Apply(CanvasToMat(canvas, c));
    }
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static Overdose instance;
  return &instance;
}
