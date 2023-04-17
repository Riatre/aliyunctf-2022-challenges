#pragma once

#include <string>
#include <memory>
#include "plugin.h"
#include "absl/status/statusor.h"

namespace hitori {

namespace intl {
  struct PluginDeleter {
    void* handle;
    void operator()(Plugin* plugin);
  };
}

using PluginHolder = std::unique_ptr<Plugin, intl::PluginDeleter>;

absl::StatusOr<PluginHolder> LoadPlugin(const char* so_path);
absl::StatusOr<PluginHolder> LoadPlugin(const std::string& so_path);

}