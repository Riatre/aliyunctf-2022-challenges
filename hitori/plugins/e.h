#pragma once

#include <memory>

namespace hitori::plugins::helpers {

class LibE {
 public:
  LibE();
  ~LibE();

  void Use();

  static LibE& GetInstance() {
    static LibE inst;
    return inst;
  }

 private:
  int* stuff_;
};

}  // namespace hitori::plugins::helpers
