#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <algorithm>
#include <optional>
#include <stdexcept>

#include "absl/log/check.h"
#include "absl/strings/escaping.h"
#include "canvas.h"
#include "fmt/core.h"
#include "plugin.h"

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
  puts("0. Load Example Image");  // TODO
  puts("1. New Empty Canvas");    // DONE
  puts("2. Resize Canvas");       // DONE
  puts("3. Show Canvas");         // DONE
  // 4.1. Draw Solid Rectangle; 4.2. Draw Solid Circle; 4.3. Blt subarea of
  // another Canvas
  puts("4. Draw on Canvas");        // TODO
  puts("5. Remove Canvas");         // DONE
  puts("6. Load Filter Plugin");    // TODO
  puts("7. Apply Filter");          // TODO
  puts("8. Unload Filter Plugin");  // TODO
  printf("> ");
}

ssize_t ReadN(void* buf, ssize_t size) {
  ssize_t i = 0;
  while (i < size) {
    ssize_t ret = read(0, reinterpret_cast<char*>(buf) + i, size - i);
    if (ret == -1) {
      break;
    }
    i += ret;
  }
  return i;
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
    printf("Integer, man! Integer! Try again: ");
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

constexpr size_t kMaxHeight = 1024;
constexpr size_t kMaxWidth = 1024;
constexpr size_t kMaxCanvasCount = 16;
constexpr size_t kMaxPluginSlot = 4;
hitori::Canvas* g_canvas[kMaxCanvasCount];
hitori::Plugin* g_plugin_slot[kMaxPluginSlot];

void LoadExampleImage() { throw std::logic_error("not implemented"); }

void NewCanvas() {
  size_t cid = 0;
  for (; cid < kMaxCanvasCount && g_canvas[cid]; cid++)
    ;
  if (cid == kMaxCanvasCount) {
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
  fmt::print("New canvas created with size {} x {}.\n", width, height);
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
  auto status = g_canvas[index]->Resize(width, height);
  if (!status.ok()) {
    fmt::print("Failed to resize canvas: {}\n", status.message());
    return;
  }
  fmt::print("Canvas resized to {} x {}.\n", width, height);
}

void ShowCanvas() {
  size_t index = ReadULongOrExit("Index: ");
  if (index > kMaxCanvasCount || !g_canvas[index] || !g_canvas[index]->Valid()) {
    puts("Invalid canvas index.");
    return;
  }
  hitori::Canvas* canvas = g_canvas[index];
  if (!canvas->Valid()) {
    puts("Canvas does not exist.");
    return;
  }
  auto png_result = canvas->ExportAsPNG();
  if (!png_result.ok()) {
    fmt::print("Failed to encode: {}\n", png_result.status().message());
    return;
  }
  fmt::print("\n\033]1337;File=inline=1;width={}px;height={}px;size={}:{}\07\n", canvas->width(),
             canvas->height(), png_result->size(), absl::Base64Escape(*png_result));
}

void DrawCanvasMenu() {
  puts("1. Draw Solid Rectangle");
  puts("2. Draw Solid Circle");
  puts("3. Blt subarea of another Canvas");
  printf("> ");
  throw std::logic_error("not implemented");
}

void RemoveCanvas() {
  size_t index = ReadULongOrExit("Index: ");
  if (index > kMaxCanvasCount || !g_canvas[index] || !g_canvas[index]->Valid()) {
    puts("Invalid canvas index.");
    return;
  }
  delete g_canvas[index];
  g_canvas[index] = nullptr;
  fmt::print("Canvas {} removed.\n", index);
}

void LoadPlugin() { throw std::logic_error("not implemented"); }

void ApplyPlugin() { throw std::logic_error("not implemented"); }

void UnloadPlugin() { throw std::logic_error("not implemented"); }

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
