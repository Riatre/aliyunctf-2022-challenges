#pragma once

#include <memory>

namespace hitori::plugins::helpers {

class LibC {
 public:
  LibC();
  ~LibC();

  void Use();

  static LibC& GetInstance() {
    static LibC inst;
    return inst;
  }

 private:
  int* stuff_;
};

}  // namespace hitori::plugins::helpers
