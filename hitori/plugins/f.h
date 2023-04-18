#pragma once

#include <memory>

namespace hitori::plugins::helpers {

class LibF {
 public:
  LibF();
  ~LibF();

  void Use();

  static LibF& GetInstance() {
    static LibF inst;
    return inst;
  }

 private:
  int* stuff_;
};

}  // namespace hitori::plugins::helpers
