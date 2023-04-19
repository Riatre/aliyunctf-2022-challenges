#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <algorithm>
#include <filesystem>
#include <optional>
#include <random>

#include "absl/log/check.h"
#include "absl/strings/escaping.h"
#include "canvas.h"
#include "fmt/core.h"
#include "plugin_manager.h"

#define EAT_AND_RETURN_IF_ERROR(expr)                  \
  do {                                                 \
    auto _status = (expr);                             \
    if (!_status.ok()) {                               \
      fmt::print("Failure: {}\n", _status.ToString()); \
      return;                                          \
    }                                                  \
  } while (0)

namespace {

void PrintBanner() {
  puts(
      "                       Hitori ~ Yet Another Cài Dān Tí ~\n"
      "Certified 100% Pure Human Made Challenge ; No Soulless AI Involved No BUG how???\n"
      "\n"
      "[Ad] RECOMMENDED CHALLENGES FOR YOU: lyla, happyropeware SOLVE THEM SOLVE THEMgg\n"
      "================================================================================\n");
}

void PrintPrompt() {
  printf(
      "0. Load Example Image\n"
      "1. New Empty Canvas\n"
      "2. Resize Canvas\n"
      "3. Show Canvas\n"
      "4. Draw on Canvas\n"
      "5. Remove Canvas\n"
      "6. Load Filter Plugin\n"
      "7. Apply Filter\n"
      "8. Unload Filter Plugin\n"
      "> ");
}

size_t ReadNUntil(void* buf, size_t size, char delim, bool* eof = nullptr) {
  size_t i = 0;
  while (i + 1 < size) {
    char c;
    int ret = read(0, &c, 1);
    if (ret != 1) {
      if (eof) *eof = true;
      break;
    }
    if (c == delim) break;
    ((char*)buf)[i++] = c;
  }
  ((char*)buf)[i] = 0;
  return i;
}

std::optional<ulong> ReadULong() {
  char buf[32];
  bool eof = false;
  while (!eof) {
    if (!ReadNUntil(buf, sizeof(buf), '\n', &eof)) continue;
    char* endptr = nullptr;
    ulong ret = strtoul(buf, &endptr, 0);
    if (endptr != buf) return ret;
    if (!eof) printf("Integer, man! Integer! Try again: ");
  }
  return {};
}

ulong ReadULongOrExit(const char* prompt = nullptr) {
  if (prompt) {
    printf("%s", prompt);
  }
  if (auto ret = ReadULong(); ret.has_value()) {
    return *ret;
  }
  exit(0);
}

std::optional<hitori::pixel_t> ParseRGB(std::string_view inp) {
  if (inp.size() != 7 || inp[0] != '#') return {};
  uint32_t parsed = 0;
  if (!absl::SimpleHexAtoi(inp.substr(1), &parsed)) return {};
  return hitori::pixel_t{(parsed >> 16) & 0xff, (parsed >> 8) & 0xff, parsed & 0xff};
}

std::optional<hitori::pixel_t> ReadRGB() {
  char buf[32];
  bool eof = false;
  while (!eof) {
    ReadNUntil(buf, sizeof(buf), '\n', &eof);
    if (auto val = ParseRGB(buf); val.has_value()) {
      return *val;
    }
    if (!eof) printf("RGB Color, guys! RGB Color! Try again: ");
  }
  return {};
}

hitori::pixel_t ReadRGBOrExit(const char* prompt = nullptr) {
  if (prompt) {
    printf("%s", prompt);
  }
  if (auto ret = ReadRGB(); ret.has_value()) {
    return *ret;
  }
  exit(0);
}

constexpr size_t kMaxHeight = 1024;
constexpr size_t kMaxWidth = 1024;
constexpr size_t kMaxCanvasCount = 16;
constexpr size_t kMaxPluginSlot = 2;
hitori::Canvas* g_canvas[kMaxCanvasCount];
hitori::PluginHolder g_plugin_slot[kMaxPluginSlot];

bool FindEmptyCanvasSlot(size_t* cid) {
  for (size_t i = 0; i < kMaxCanvasCount; i++) {
    if (!g_canvas[i]) {
      *cid = i;
      return true;
    }
  }
  return false;
}

void LoadExampleImage() {
  size_t cid = 0;
  if (!FindEmptyCanvasSlot(&cid)) {
    puts("Maximum canvas count exceeded.");
    return;
  }
  if (!std::filesystem::is_directory("assets")) {
    puts("No example image found.");
    return;
  }
  std::vector<std::filesystem::path> candidates;
  for (const auto& entry : std::filesystem::directory_iterator("assets")) {
    if (!entry.is_regular_file()) continue;
    if (entry.path().extension() != ".png") continue;
    candidates.push_back(entry.path());
  }
  if (candidates.empty()) {
    puts("No example image found.");
    return;
  }
  // Choose a random image.
  std::random_device rng;
  size_t idx = std::uniform_int_distribution<size_t>(0, candidates.size() - 1)(rng);
  auto result = hitori::Canvas::FromPNG(candidates[idx]);
  EAT_AND_RETURN_IF_ERROR(result.status());
  g_canvas[cid] = new hitori::Canvas();
  *g_canvas[cid] = std::move(result.value());
  fmt::print("Loaded {} to canvas #{}.\n", candidates[idx].stem().string(), cid);
}

void NewCanvas() {
  size_t cid = 0;
  if (!FindEmptyCanvasSlot(&cid)) {
    puts("Maximum canvas count exceeded.");
    return;
  }

  ulong width = ReadULongOrExit("Width: ");
  ulong height = ReadULongOrExit("Height: ");
  if (width == 0 || width > kMaxWidth || height == 0 || height > kMaxHeight) {
    puts("Invalid dimension.");
    return;
  }
  g_canvas[cid] = new hitori::Canvas(width, height);
  CHECK(g_canvas[cid]->Valid());
  fmt::print("Canvas #{} created with size {} x {}.\n", cid, width, height);
}

void ResizeCanvas() {
  size_t index = ReadULongOrExit("Index: ");
  if (index >= kMaxCanvasCount) {
    puts("Invalid canvas index.");
    return;
  }
  if (!g_canvas[index] || !g_canvas[index]->Valid()) {
    puts("Canvas does not exist.");
    return;
  }
  ulong width = ReadULongOrExit("New width: ");
  ulong height = ReadULongOrExit("New height: ");
  if (width == 0 || width > kMaxWidth || height == 0 || height > kMaxHeight) {
    puts("Invalid dimension.");
    return;
  }
  EAT_AND_RETURN_IF_ERROR(g_canvas[index]->Resize(width, height));
  fmt::print("Canvas #{} resized to {} x {}.\n", index, width, height);
}

void ShowCanvas() {
  size_t index = ReadULongOrExit("Index: ");
  if (index >= kMaxCanvasCount || !g_canvas[index] || !g_canvas[index]->Valid()) {
    puts("Invalid canvas index.");
    return;
  }
  hitori::Canvas& canvas = *g_canvas[index];
  auto png_result = canvas.ExportAsPNG();
  EAT_AND_RETURN_IF_ERROR(png_result.status());
  // Display the image via iTerm2 inline image protocol. It base64 so still counts as plain text
  // right, right???
  fmt::print("\n\033]1337;File=inline=1;width={}px;height={}px;size={}:{}\07\n", canvas.width(),
             canvas.height(), png_result->size(), absl::Base64Escape(*png_result));
}

void DrawCanvasMenu() {
  size_t index = ReadULongOrExit("Index: ");
  if (index >= kMaxCanvasCount || !g_canvas[index] || !g_canvas[index]->Valid()) {
    puts("Invalid canvas index.");
    return;
  }
  size_t choice = ReadULongOrExit(
      "1. Draw Solid Rectangle\n"
      "2. Draw Solid Circle\n"
      "3. Blt subarea of another Canvas\n"
      "> ");
  size_t x = ReadULongOrExit("X: ");
  size_t y = ReadULongOrExit("Y: ");
  hitori::Canvas* canvas = g_canvas[index];
  switch (choice) {
    case 1: {
      size_t w = ReadULongOrExit("Width: ");
      size_t h = ReadULongOrExit("Height: ");
      hitori::pixel_t color = ReadRGBOrExit("Color: ");
      EAT_AND_RETURN_IF_ERROR(canvas->DrawSolidRectangle(x, y, w, h, color));
      break;
    }
    case 2: {
      size_t r = ReadULongOrExit("Radius: ");
      hitori::pixel_t color = ReadRGBOrExit("Color: ");
      EAT_AND_RETURN_IF_ERROR(canvas->DrawSolidCircle(x, y, r, color));
      break;
    }
    case 3: {
      size_t src_index = ReadULongOrExit("Source Canvas Index: ");
      if (src_index >= kMaxCanvasCount || !g_canvas[src_index] || !g_canvas[src_index]->Valid()) {
        puts("Invalid canvas index.");
        return;
      }
      size_t src_x = ReadULongOrExit("Source X: ");
      size_t src_y = ReadULongOrExit("Source Y: ");
      size_t src_w = ReadULongOrExit("Source Width: ");
      size_t src_h = ReadULongOrExit("Source Height: ");
      auto src_view = g_canvas[src_index]->Sub(src_x, src_y, src_w, src_h);
      EAT_AND_RETURN_IF_ERROR(src_view.status());
      EAT_AND_RETURN_IF_ERROR(canvas->Blt(x, y, *src_view));
      break;
    }
    default:
      puts("?");
      return;
  }
}

void RemoveCanvas() {
  size_t index = ReadULongOrExit("Index: ");
  if (index >= kMaxCanvasCount || !g_canvas[index]) {
    puts("Invalid canvas index.");
    return;
  }
  delete g_canvas[index];
  g_canvas[index] = nullptr;
  fmt::print("Canvas #{} removed.\n", index);
}

void LoadPlugin() {
  size_t slot_id = 0;
  for (; slot_id < kMaxPluginSlot; ++slot_id) {
    if (!g_plugin_slot[slot_id]) break;
  }
  if (slot_id == kMaxPluginSlot) {
    puts("No more plugin slot.");
    return;
  }

  char namebuf[256];
  printf("Plugin name: ");
  size_t namelen = ReadNUntil(namebuf, sizeof(namebuf), '\n');
  std::string_view name(namebuf, namelen);
  if (name.find_first_of('.') != std::string_view::npos ||
      name.find_first_of('/') != std::string_view::npos) {
    name = "dQw4w9WgXcQ";
  }
  auto so_path = fmt::format("plugins/{}.so", name);
  if (!std::filesystem::exists(so_path)) {
    fmt::print("Plugin {} not found.\n", name);
    return;
  }
  auto result = hitori::LoadPlugin(so_path);
  EAT_AND_RETURN_IF_ERROR(result.status());
  g_plugin_slot[slot_id] = std::move(result.value());
  fmt::print("Plugin {} ({}) loaded to slot #{}.\n{}\n", g_plugin_slot[slot_id]->Name(), so_path,
             slot_id, g_plugin_slot[slot_id]->Description());
}

void ApplyPlugin() {
  size_t index = ReadULongOrExit("Plugin Index: ");
  if (index >= kMaxPluginSlot || !g_plugin_slot[index]) {
    puts("Invalid plugin slot.");
    return;
  }
  hitori::Plugin& plugin = *g_plugin_slot[index];
  size_t cid;
  switch (plugin.Type()) {
    case hitori::PluginType::FILTER_PLUGIN:
      cid = ReadULongOrExit("Canvas Index: ");
      if (cid >= kMaxCanvasCount || !g_canvas[cid] || !g_canvas[cid]->Valid()) {
        puts("Invalid canvas index.");
        return;
      }
      break;

    case hitori::PluginType::GENERATIVE_PLUGIN:
      if (!FindEmptyCanvasSlot(&cid)) {
        puts("Maximum canvas count exceeded.");
        return;
      }
      g_canvas[cid] = new hitori::Canvas();
      break;

    default:
      puts("Unknown plugin type.");
      return;
  }
  EAT_AND_RETURN_IF_ERROR(plugin.Apply(*g_canvas[cid]));
  fmt::print("Plugin {} applied to canvas #{}.\n", g_plugin_slot[index]->Name(), cid);
}

void UnloadPlugin() {
  size_t index = ReadULongOrExit("Index: ");
  if (index >= kMaxPluginSlot || !g_plugin_slot[index]) {
    puts("Invalid plugin slot.");
    return;
  }
  std::string name = g_plugin_slot[index]->Name();
  g_plugin_slot[index].reset();
  fmt::print("Plugin {} (slot #{}) unloaded.\n", name, index);
}

}  // namespace

int main(int argc, char* argv[]) {
  setvbuf(stdin, nullptr, _IONBF, 0);
  setvbuf(stdout, nullptr, _IONBF, 0);
  setvbuf(stderr, nullptr, _IONBF, 0);

  PrintBanner();
  while (true) {
    PrintPrompt();
    size_t choice = ReadULongOrExit();
    static decltype(NewCanvas)* handlers[] = {LoadExampleImage, NewCanvas,      ResizeCanvas,
                                              ShowCanvas,       DrawCanvasMenu, RemoveCanvas,
                                              LoadPlugin,       ApplyPlugin,    UnloadPlugin};
    if (choice >= sizeof(handlers) / sizeof(handlers[0])) {
      puts("?");
      continue;
    }
    handlers[choice]();
  }
  return 0;
}
