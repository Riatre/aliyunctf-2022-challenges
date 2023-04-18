#pragma once

#include <memory>

namespace hitori::plugins::helpers {

class LibA {
 public:
  LibA();
  ~LibA();

  void Use();

  static LibA& GetInstance() {
    static LibA inst;
    return inst;
  }

 private:
  int* stuff_;
};

}  // namespace hitori::plugins::helpers
