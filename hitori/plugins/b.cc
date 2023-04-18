#include "b.h"

#include <cstdlib>

namespace hitori::plugins::helpers {

LibB::LibB() : stuff_(new int(42)) {}

LibB::~LibB() { delete stuff_; }

void LibB::Use() {
  srand(*stuff_);
  *stuff_ = 1;
}

}  // namespace hitori::plugins::helpers
