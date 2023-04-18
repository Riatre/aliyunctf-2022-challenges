#include "plugin.h"

namespace {

class FilterC : public hitori::Plugin {
 public:
  hitori::PluginType Type() const override { return hitori::PluginType::FILTER_PLUGIN; }
  std::string Name() const override { return "Filter C"; }
  std::string Description() const override { return "This is a filter plugin."; }
  absl::Status Apply(hitori::Canvas& canvas) const override { return absl::OkStatus(); }
};

}  // namespace

extern "C" hitori::Plugin* GetPlugin() {
  static FilterC instance;
  return &instance;
}
