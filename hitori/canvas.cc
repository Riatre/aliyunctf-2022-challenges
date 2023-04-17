#include "canvas.h"

#include <algorithm>

#include "absl/log/check.h"
#include "spng.h"

namespace {

template <typename T>
constexpr T SaturatingAdd(T a, T b, T maxv = std::numeric_limits<T>::max()) {
  static_assert(std::is_unsigned_v<T>);
  T result;
  return (__builtin_add_overflow(a, b, &result) || result > maxv) ? maxv : result;
}

}  // namespace

namespace hitori {

Canvas::Canvas() : width_(0), height_(0), data_(nullptr) {}
Canvas::Canvas(size_t width, size_t height) : width_(width), height_(height) {
  size_t alloc_size = 0;
  if (__builtin_mul_overflow(width, height, &alloc_size) ||
      __builtin_mul_overflow(kChannels, alloc_size, &alloc_size)) {
    throw std::bad_alloc();
  }
  data_ = new color_t[alloc_size];
  std::fill(data_, data_ + alloc_size, 255);
}
Canvas::~Canvas() { delete[] data_; }

absl::Status Canvas::Resize(size_t width, size_t height) {
  size_t alloc_size = 0;
  if (__builtin_mul_overflow(width, height, &alloc_size) ||
      __builtin_mul_overflow(kChannels, alloc_size, &alloc_size)) {
    return absl::InvalidArgumentError("invalid size");
  }
  if (width_ == width && height_ == height) {
    return absl::OkStatus();
  }
  delete[] data_;
  width_ = width;
  height_ = height;
  data_ = new color_t[alloc_size];
  return absl::OkStatus();
}

absl::Status Canvas::DrawSolidRectangle(size_t x, size_t y, size_t w, size_t h, pixel_t px) {
  if (x >= width_ || y >= height_) {
    return absl::InvalidArgumentError("Rectangle exceeds canvas boundaries");
  }
  size_t y_end = SaturatingAdd(y, h, height_);
  size_t x_end = SaturatingAdd(x, w, width_);
  for (size_t i = y; i < y_end; i++) {
    for (size_t j = x; j < x_end; j++) {
      SetPixel(j, i, px);
    }
  }
  return absl::OkStatus();
}

absl::Status Canvas::DrawSolidCircle(size_t x, size_t y, size_t radius, pixel_t color) {
  if (x >= width_ || y >= height_) {
    return absl::InvalidArgumentError("Circle center exceeds canvas boundaries");
  }
  size_t y_start = (y >= radius) ? y - radius : 0;
  size_t y_end = SaturatingAdd(y, radius, height_);
  size_t x_start = (x >= radius) ? x - radius : 0;
  size_t x_end = SaturatingAdd(x, radius, width_);
  for (size_t i = y_start; i < y_end; i++) {
    for (size_t j = x_start; j < x_end; j++) {
      if ((i - y) * (i - y) + (j - x) * (j - x) <= radius * radius) {
        SetPixel(j, i, color);
      }
    }
  }
  return absl::OkStatus();
}

absl::Status Canvas::Blt(size_t x, size_t y, CanvasView other) {
  if (x >= width_ || y >= height_) {
    return absl::InvalidArgumentError("Destination exceeds canvas boundaries");
  }
  size_t y_end = SaturatingAdd(y, other.height(), height_);
  size_t x_end = SaturatingAdd(x, other.width(), width_);
  for (size_t i = y; i < y_end; i++) {
    for (size_t j = x; j < x_end; j++) {
      SetPixel(j, i, other.GetPixel(j - x, i - y));
    }
  }
  return absl::OkStatus();
}

absl::StatusOr<ExportedCanvasBuffer> Canvas::ExportAsPNG() const {
  spng_ctx* ctx = spng_ctx_new(SPNG_CTX_ENCODER);
  spng_set_option(ctx, SPNG_ENCODE_TO_BUFFER, 1);
  spng_ihdr ihdr = {
      .width = static_cast<uint32_t>(width_),
      .height = static_cast<uint32_t>(height_),
      .bit_depth = 8,
      .color_type = SPNG_COLOR_TYPE_TRUECOLOR,
  };
  spng_set_ihdr(ctx, &ihdr);
  spng_encode_image(ctx, data_, ByteSize(), SPNG_FMT_PNG, SPNG_ENCODE_FINALIZE);
  size_t png_size;
  int error;
  void* png_data = spng_get_png_buffer(ctx, &png_size, &error);
  if (!png_data) {
    return absl::InternalError(spng_strerror(error));
  }
  spng_ctx_free(ctx);
  return ExportedCanvasBuffer(png_data, png_size);
}

absl::StatusOr<CanvasView> Canvas::Sub(size_t x, size_t y, size_t w, size_t h) const {
  if (x >= width_ || y >= height_) {
    return absl::InvalidArgumentError("Subcanvas exceeds canvas boundaries");
  }
  w = std::min(w, width_ - x);
  h = std::min(h, height_ - y);
  return CanvasView(*this, x, y, w, h);
}

}  // namespace hitori
