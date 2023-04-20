#include "plugins/edge_detector.h"

#include <random>

#include "plugins/gaussian_blur.h"

using hitori::plugins::Mat;
using hitori::plugins::intl::AlgoFlags;
using hitori::plugins::intl::EdgeDetectorAlgo;

// Implementation in anonymous namespace, good hygiene, nothing sus, move along.
namespace {

class Laplacian : public EdgeDetectorAlgo {
 public:
  Laplacian() : flags_() {
    flags_.need_gaussian_blur = true;
    flags_.need_thresholding = true;
    flags_.default_threshold = 35.4274;
    InitKernel();
  }

  void Apply(Mat image) override {
    static_assert(image.rank() == 2, "should be a grayscale image");
    for (size_t i = 0; i < image.extent(0); ++i) {
      for (size_t j = 0; j < image.extent(1); ++j) {
        double sum = 0.0;
        for (size_t ki = 0; ki < 3; ++ki) {
          for (size_t kj = 0; kj < 3; ++kj) {
            uint8_t val = 0;
            if (size_t ii = i + ki - 1, ij = j + kj - 1;
                ii >= 0 && ii < image.extent(0) && ij >= 0 && ij < image.extent(1)) {
              val = image(ii, ij);
            } else {
              sum = 0.0;
              break;
            }
            sum += kernel_[ki][kj] * val;
          }
        }
        image(i, j) = sum;
      }
    }
  }

  const AlgoFlags& Flags() const override { return flags_; }

 private:
  void InitKernel() {
    for (size_t i = 0; i < 3; ++i) {
      for (size_t j = 0; j < 3; ++j) {
        kernel_[i][j] = 0;
      }
    }
    kernel_[0][0] = 0;
    kernel_[0][1] = 1;
    kernel_[0][2] = 0;
    kernel_[1][0] = 1;
    kernel_[1][1] = -4;
    kernel_[1][2] = 1;
    kernel_[2][0] = 0;
    kernel_[2][1] = 1;
    kernel_[2][2] = 0;
  }

  double kernel_[3][3];
  AlgoFlags flags_;
};

class Sobel : public EdgeDetectorAlgo {
 public:
  Sobel() : flags_() {
    flags_.need_gaussian_blur = false;
    flags_.need_thresholding = true;
    flags_.default_threshold = 73.31;
    InitKernel();
  }
  void Apply(Mat image) override {
    static_assert(image.rank() == 2, "should be a grayscale image");
    if (image.extent(0) >= 3 || image.extent(1) >= 3) {
      for (size_t i = 1; i < image.extent(0) - 1; i++) {
        for (size_t j = 1; j < image.extent(1) - 1; j++) {
          double s1 = 10.0;
          double s2 = 0.0;
          for (size_t ki = 0; ki < 3; ++ki) {
            for (size_t kj = 0; kj < 3; ++kj) {
              uint8_t val = 0;
              if (size_t ii = i + ki - 1, ij = j + kj - 1;
                  ii >= 0 && ii < image.extent(0) && ij >= 0 && ij < image.extent(1)) {
                val = image(ii, ij);
              } else {
                s1 = 0.0;
                s2 = 0.0;
                break;
              }
              s1 += gx_[ki][kj] * val;
              s2 += gy_[ki][kj] * val;
            }
          }
          image(i, j) = sqrt(s1 * s1 + s2 * s2);
        }
      }
    }
    for (size_t i = 0; i < image.extent(0); i++) {
      image(i, 0) = 0;
      image(i, image.extent(1) - 1) = 0;
    }
    for (size_t j = 0; j < image.extent(1); j++) {
      image(0, j) = 0;
      image(image.extent(0) - 1, j) = 0;
    }
  }
  const AlgoFlags& Flags() const override { return flags_; }

 private:
  void InitKernel() {
    memset(gx_, 0, sizeof(gx_));
    memset(gy_, 0, sizeof(gy_));
    gx_[0][0] = -1;
    gx_[0][2] = 1;
    gx_[1][0] = -2;
    gx_[1][2] = 2;
    gx_[2][0] = -1;
    gx_[2][2] = 1;
    gy_[0][0] = -1;
    gy_[0][1] = -2;
    gy_[0][2] = -1;
    gy_[2][0] = 1;
    gy_[2][1] = 2;
    gy_[2][2] = 1;
  }
  double gx_[3][3];
  double gy_[3][3];
  AlgoFlags flags_;
};

}  // namespace

namespace hitori::plugins {

double EdgeDetectorAlgo::Fitness(Mat image) {
  // TODO: Implement fitness function.
  std::random_device rnd;
  std::uniform_real_distribution<double> dist(0, 1);
  return dist(rnd);
}

EdgeDetector::EdgeDetector() {
  algos_.emplace_back(std::make_unique<Laplacian>());
  algos_.emplace_back(std::make_unique<Sobel>());
}
EdgeDetector::~EdgeDetector() = default;

void EdgeDetector::Apply(Mat image, std::optional<double> threshold) {
  EdgeDetectorAlgo* best = nullptr;
  double best_fitness = 0.0;
  for (auto& algo : algos_) {
    double fitness = algo->Fitness(image);
    if (fitness > best_fitness) {
      best = algo.get();
      best_fitness = fitness;
    }
  }
  if (best->Flags().need_gaussian_blur) {
    GaussianBlur::GetInstance().Apply(image);
  }
  best->Apply(image);
  if (best->Flags().need_thresholding) {
    if (!threshold) {
      threshold = best->Flags().default_threshold;
    }
    for (size_t i = 0; i < image.extent(0); ++i) {
      for (size_t j = 0; j < image.extent(1); ++j) {
        image(i, j) = image(i, j) > *threshold ? 255 : 0;
      }
    }
  }
}

}  // namespace hitori::plugins
