#include "plugin.h"
#include "a.h"
#include "d.h"

namespace {

class FilterA : public hitori::Plugin {
 public:
  ~FilterA() override = default;
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Filter A"; }
  std::string Description() const override { return "This is a filter plugin."; }
  absl::Status Apply(hitori::Canvas& canvas) const override {
    hitori::plugins::helpers::LibA::GetInstance().Use();
    hitori::plugins::helpers::LibD::GetInstance().Use();
    return absl::OkStatus();
  }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static FilterA instance;
  return &instance;
}
