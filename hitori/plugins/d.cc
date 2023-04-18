#include "d.h"

#include <cstdlib>

namespace hitori::plugins::helpers {

LibD::LibD() : stuff_(new int(42)) {}

LibD::~LibD() { delete stuff_; }

void LibD::Use() {
  srand(*stuff_);
  *stuff_ = 1;
}

}  // namespace hitori::plugins::helpers
