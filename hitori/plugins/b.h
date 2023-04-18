#pragma once

#include <memory>

namespace hitori::plugins::helpers {

class LibB {
 public:
  LibB();
  ~LibB();

  void Use();

  static LibB& GetInstance() {
    static LibB inst;
    return inst;
  }

 private:
  int* stuff_;
};

}  // namespace hitori::plugins::helpers
