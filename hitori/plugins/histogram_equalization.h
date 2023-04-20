#pragma once

#include <memory>

#include "plugins/image_utils.h"

namespace hitori::plugins {

class HistogramEqualization {
 public:
  HistogramEqualization();
  ~HistogramEqualization();

  HistogramEqualization(const HistogramEqualization&) = delete;
  HistogramEqualization& operator=(const HistogramEqualization&) = delete;
  HistogramEqualization(HistogramEqualization&&) = delete;
  HistogramEqualization& operator=(HistogramEqualization&&) = delete;

  void GlobalHistogramEqualization(Mat image) const;

  static HistogramEqualization& GetInstance() {
    static HistogramEqualization inst;
    return inst;
  }
};

}  // namespace hitori::plugins
