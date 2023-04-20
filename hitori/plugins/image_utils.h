#pragma once

#include <mdspan>

#include "canvas.h"

namespace stdex = std::experimental;

namespace hitori::plugins {

using Mat = stdex::mdspan<uint8_t, stdex::dextents<size_t, 2>, stdex::layout_stride>;
using HWCMat =
    stdex::mdspan<uint8_t, stdex::extents<size_t, stdex::dynamic_extent, stdex::dynamic_extent, 3>>;
using CHWMat =
    stdex::mdspan<uint8_t, stdex::extents<size_t, 3, stdex::dynamic_extent, stdex::dynamic_extent>,
                  stdex::layout_stride>;

Mat CanvasToMat(Canvas& canvas, size_t channel_index);
HWCMat CanvasToHWC(Canvas& canvas);
CHWMat CanvasToCHW(Canvas& canvas);

}  // namespace hitori::plugins
