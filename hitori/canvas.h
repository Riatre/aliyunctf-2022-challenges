#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace hitori {

using color_t = uint8_t;
inline constexpr size_t kChannels = 3;

namespace intl {

struct CFreeDeleter {
  void operator()(void* ptr) const { free(ptr); }
};

}  // namespace intl

class CanvasView;

class ExportedCanvasBuffer {
 public:
  ExportedCanvasBuffer() : data_(nullptr), size_(0) {}
  ExportedCanvasBuffer(void* data, size_t size) : data_(data), size_(size) {}
  operator std::string_view() const {
    return std::string_view(reinterpret_cast<const char*>(data_.get()), size_);
  }
  void* data() const { return data_.get(); }
  size_t size() const { return size_; }

 private:
  std::unique_ptr<void, intl::CFreeDeleter> data_;
  size_t size_;
};

class Canvas {
 public:
  Canvas();
  Canvas(size_t width, size_t height);
  ~Canvas() { delete[] data_; }
  bool Valid() const { return data_ != nullptr; }
  size_t width() const { return width_; }
  size_t height() const { return height_; }
  color_t* data() { return data_; }
  const color_t* data() const { return data_; }
  size_t ByteSize() const { return kChannels * width_ * height_; }
  absl::Status Resize(size_t width, size_t height);
  absl::Status DrawSolidRectangle();
  absl::Status DrawSolidCircle();
  absl::Status Blt(size_t x, size_t y, CanvasView other);
  absl::StatusOr<ExportedCanvasBuffer> ExportAsPNG() const;

 private:
  size_t width_;
  size_t height_;
  // Image data in dense (C, H, W) layout.
  color_t* data_;
};

class CanvasView {
  // ...
};

}  // namespace hitori
