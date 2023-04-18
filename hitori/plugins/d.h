#pragma once

#include <memory>

namespace hitori::plugins::helpers {

class LibD {
 public:
  LibD();
  ~LibD();

  void Use();

  static LibD& GetInstance() {
    static LibD inst;
    return inst;
  }

 private:
  int* stuff_;
};

}  // namespace hitori::plugins::helpers
