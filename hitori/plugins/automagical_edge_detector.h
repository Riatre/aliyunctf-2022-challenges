#pragma once

#include <optional>

#include "plugins/image_utils.h"

namespace hitori::plugins {

namespace intl {

struct AlgoFlags {
  AlgoFlags() : need_gaussian_blur(false), need_thresholding(false) {}

  bool need_gaussian_blur : 1;
  bool need_thresholding : 1;
  double default_threshold = 0;
};

class EdgeDetectorAlgo {
 public:
  virtual ~EdgeDetectorAlgo() = default;
  // We run the algo which fits the image best.
  virtual double Fitness(helpers::Mat image);
  virtual void Apply(helpers::Mat image) = 0;
  virtual const AlgoFlags& Flags() const = 0;
};

}  // namespace intl

class EdgeDetector {
 public:
  EdgeDetector();
  ~EdgeDetector();

  EdgeDetector(const EdgeDetector&) = delete;
  EdgeDetector& operator=(const EdgeDetector&) = delete;
  EdgeDetector(EdgeDetector&&) = delete;
  EdgeDetector& operator=(EdgeDetector&&) = delete;

  void Apply(helpers::Mat image, std::optional<double> threshold = std::nullopt);

  static EdgeDetector& GetInstance() {
    static EdgeDetector inst;
    return inst;
  }

 private:
  std::vector<std::unique_ptr<intl::EdgeDetectorAlgo>> algos_;
};

}  // namespace hitori::plugins
