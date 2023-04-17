#include "plugin_manager.h"

#include <dlfcn.h>

namespace hitori {

namespace intl {
void PluginDeleter::operator()(Plugin* plugin) {
  if (plugin && handle) {
    dlclose(handle);
    handle = nullptr;
  }
}
}  // namespace intl

absl::StatusOr<PluginHolder> LoadPlugin(const char* so_path) {
  void* handle = dlopen(so_path, RTLD_LAZY | RTLD_LOCAL);
  if (!handle) {
    return absl::InternalError(dlerror());
  }
  auto get_plugin = reinterpret_cast<Plugin* (*)()>(dlsym(handle, "GetPlugin"));
  if (!get_plugin) {
    std::string error = dlerror();
    dlclose(handle);
    return absl::InternalError(error);
  }
  Plugin* plugin = get_plugin();
  if (!plugin) {
    dlclose(handle);
    return absl::InternalError("GetPlugin returned nullptr");
  }
  return PluginHolder(plugin, {handle});
}

absl::StatusOr<PluginHolder> LoadPlugin(const std::string& so_path) {
  return LoadPlugin(so_path.c_str());
}

}  // namespace hitori
