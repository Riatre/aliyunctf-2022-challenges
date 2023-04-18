#include "c.h"

#include <cstdlib>

namespace hitori::plugins::helpers {

LibC::LibC() : stuff_(new int(42)) {}

LibC::~LibC() { delete stuff_; }

void LibC::Use() {
  srand(*stuff_);
  *stuff_ = 1;
}

}  // namespace hitori::plugins::helpers
