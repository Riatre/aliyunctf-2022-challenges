#include "c.h"
#include "f.h"
#include "plugin.h"

namespace {

class FilterC : public hitori::Plugin {
 public:
  ~FilterC() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Filter C"; }
  std::string Description() const override { return "This is a filter plugin."; }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    hitori::plugins::helpers::LibC::GetInstance().Use();
    hitori::plugins::helpers::LibF::GetInstance().Use();
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static FilterC instance;
  return &instance;
}
