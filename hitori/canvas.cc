#include "canvas.h"

#include <algorithm>

#include "spng.h"

namespace hitori {

Canvas::Canvas() : width_(0), height_(0), data_(nullptr) {}
Canvas::Canvas(size_t width, size_t height) : width_(width), height_(height) {
  size_t alloc_size = 0;
  if (__builtin_mul_overflow(width, height, &alloc_size) ||
      __builtin_mul_overflow(kChannels, alloc_size, &alloc_size)) {
    throw std::bad_alloc();
  }
  data_ = new color_t[kChannels * width_ * height_];
  std::fill(data_, data_ + kChannels * width_ * height_, 255);
}

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
  data_ = new color_t[kChannels * width_ * height_];
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

}  // namespace hitori
