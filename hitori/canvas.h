#pragma once
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <filesystem>

#include "absl/status/status.h"
#include "absl/status/statusor.h"

namespace hitori {

using color_t = uint8_t;
using pixel_t = std::tuple<color_t, color_t, color_t>;
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
  ~Canvas();
  Canvas(const Canvas&);
  Canvas& operator=(const Canvas&);
  Canvas(Canvas&&);
  Canvas& operator=(Canvas&&);

  bool Valid() const { return data_ != nullptr; }
  size_t width() const { return width_; }
  size_t height() const { return height_; }
  color_t* data() { return data_; }
  const color_t* data() const { return data_; }
  size_t ByteSize() const { return kChannels * width_ * height_; }
  absl::Status Resize(size_t width, size_t height);
  absl::Status DrawSolidRectangle(size_t x, size_t y, size_t w, size_t h, pixel_t color);
  absl::Status DrawSolidCircle(size_t x, size_t y, size_t radius, pixel_t color);
  absl::Status Blt(size_t x, size_t y, CanvasView other);
  absl::StatusOr<ExportedCanvasBuffer> ExportAsPNG() const;
  static absl::StatusOr<Canvas> FromPNG(const std::filesystem::path& path);
  pixel_t GetPixel(size_t x, size_t y) const {
    return {data_[y * width_ * kChannels + x * kChannels + 0],
            data_[y * width_ * kChannels + x * kChannels + 1],
            data_[y * width_ * kChannels + x * kChannels + 2]};
  }
  void SetPixel(size_t x, size_t y, pixel_t px) {
    data_[y * width_ * kChannels + x * kChannels + 0] = std::get<0>(px);
    data_[y * width_ * kChannels + x * kChannels + 1] = std::get<1>(px);
    data_[y * width_ * kChannels + x * kChannels + 2] = std::get<2>(px);
  }
  absl::StatusOr<CanvasView> Sub(size_t x, size_t y, size_t w = std::string::npos,
                                 size_t h = std::string::npos) const;

 private:
  size_t width_;
  size_t height_;
  // Image data in dense (H, W, C) layout.
  color_t* data_;
};

class CanvasView {
 public:
  CanvasView(const Canvas& canvas, size_t x, size_t y, size_t w, size_t h)
      : canvas_(&canvas), x_(x), y_(y), w_(w), h_(h) {}
  CanvasView(const Canvas& canvas) : CanvasView(canvas, 0, 0, canvas.width(), canvas.height()) {}
  size_t width() const { return w_; }
  size_t height() const { return h_; }
  pixel_t GetPixel(size_t x, size_t y) const { return canvas_->GetPixel(x_ + x, y_ + y); }

 private:
  const Canvas* canvas_;
  size_t x_;
  size_t y_;
  size_t w_;
  size_t h_;
};

}  // namespace hitori
