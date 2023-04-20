#include "canvas.h"
#include "plugin.h"
#include "plugins/enhancer.h"
#include "plugins/image_utils.h"

namespace {

using namespace hitori::plugins;

class MTXX : public hitori::Plugin {
 public:
  ~MTXX() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Beautiful Rabbit Sniff Sniff"; }
  std::string Description() const override {
    return "MTXX. Beautiful Rabbit Sniff Sniff. If you still don't get it then go away, "
           "understanding dank meme is not part of the challenge.";
  }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    auto im = CanvasToHWC(canvas);
    Enhancer::GetInstance().IncreaseSaturation(im, 1.337);
    Enhancer::GetInstance().Brighten(im, 0.8954);
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static MTXX instance;
  return &instance;
}
