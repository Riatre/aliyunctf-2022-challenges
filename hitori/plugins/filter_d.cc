#include "d.h"
#include "e.h"
#include "f.h"
#include "plugin.h"

namespace {

class FilterD : public hitori::Plugin {
 public:
  ~FilterD() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Filter D"; }
  std::string Description() const override { return "This is a filter plugin."; }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    hitori::plugins::helpers::Grayscale::GetInstance().Use();
    hitori::plugins::helpers::Sobel::GetInstance().Use();
    hitori::plugins::helpers::LibF::GetInstance().Use();
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static FilterD instance;
  return &instance;
}
