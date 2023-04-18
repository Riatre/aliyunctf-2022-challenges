#include "a.h"

#include <cstdlib>

namespace hitori::plugins::helpers {

LibA::LibA() : stuff_(new int(42)) {}

LibA::~LibA() { delete stuff_; }

void LibA::Use() {
  srand(*stuff_);
  *stuff_ = 1;
}

}  // namespace hitori::plugins::helpers
