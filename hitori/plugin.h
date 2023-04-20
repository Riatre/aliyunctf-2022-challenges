#pragma once
#include <cstdint>
#include <string_view>

#include "absl/status/status.h"
#include "canvas.h"

namespace hitori {

enum class PluginType : uint32_t {
  INVALID_PLUGIN = 0,
  FILTER_PLUGIN,
  GENERATIVE_PLUGIN,
};

class Plugin {
 public:
  virtual ~Plugin() = default;
  virtual PluginType Type() const = 0;
  virtual std::string Name() const = 0;
  virtual std::string Description() const = 0;
  virtual absl::Status Apply(Canvas& canvas) const = 0;
};

}  // namespace hitori
