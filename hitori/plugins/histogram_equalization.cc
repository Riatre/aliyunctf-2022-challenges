#include "plugins/histogram_equalization.h"

#include <algorithm>

#include "plugins/image_utils.h"

namespace hitori::plugins::helpers {

HistogramEqualization::HistogramEqualization() = default;
HistogramEqualization::~HistogramEqualization() = default;

// Definitely human written not AI generated nonsense I promise (blink).
void HistogramEqualization::GlobalHistogramEqualization(Mat image) const {
  // Calculate the histogram.
  size_t histogram[256] = {0};
  for (size_t i = 0; i < image.extent(0); i++) {
    for (size_t j = 0; j < image.extent(1); j++) {
      histogram[image(i, j)]++;
    }
  }

  // Calculate the cumulative histogram.
  size_t cumulative_histogram[256] = {0};
  cumulative_histogram[0] = histogram[0];
  for (size_t i = 1; i < 256; i++) {
    cumulative_histogram[i] = cumulative_histogram[i - 1] + histogram[i];
  }

  // Calculate the equalized histogram.
  size_t size = image.extent(0) * image.extent(1) - cumulative_histogram[0];
  size_t equalized_histogram[256] = {0};
  for (size_t i = 0; i < 256; i++) {
    equalized_histogram[i] = std::round(255 * (cumulative_histogram[i] - cumulative_histogram[0]) /
                                        static_cast<double>(size));
  }

  // Apply the equalized histogram to the image.
  for (size_t i = 0; i < image.extent(0); i++) {
    for (size_t j = 0; j < image.extent(1); j++) {
      image(i, j) = equalized_histogram[image(i, j)];
    }
  }
}

}  // namespace hitori::plugins::helpers
